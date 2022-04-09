//
// Created by ousing9 on 2022/3/9.
//

#include <algorithm>
#include <fcntl.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fstream>
#include "config.h"
#include "data_manager.h"
#include "data_processor.h"
#include "file.h"
#include "time.h"
using namespace storj;

data_manager::data_manager()
{
    init();
}

data_manager::~data_manager()
{
    // 关闭数据库
    sqlite3_close_v2(sql);
}

void data_manager::init()
{
    // 初始化数据库
    init_db();
    // 初始化存储节点
    init_storage_nodes();
}

long gettimens()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return ts.tv_sec*1000000000+ts.tv_nsec;
	
}

void data_manager::init_db()
{
    const char *sql_create_table_file = "create table if not exists \"file\"\n"
                                        "(\n"
                                        "    \"id\"                 varchar(64) primary key not null,\n"
                                        "    \"file_name\"          varchar(255)            not null,\n"
                                        "    \"file_size\"          int(11)                 not null,\n"
                                        "    \"segment_size\"       int(11)                 not null,\n"
                                        "    \"stripe_size\"        int(11)                 not null,\n"
                                        "    \"erasure_share_size\" int(11)                 not null,\n"
                                        "    \"k\"                  int(11)                 not null,\n"
                                        "    \"m\"                  int(11)                 not null,\n"
                                        "    \"n\"                  int(11)                 not null\n"
                                        ");";
    const char *sql_create_table_segment = "create table if not exists \"segment\"\n"
                                           "(\n"
                                           "    \"id\"      varchar(64) primary key not null,\n"
                                           "    \"index\"   int(11)                 not null,\n"
                                           "    \"file_id\" varchar(64)             not null\n"
                                           ");";
    const char *sql_create_table_piece = "create table if not exists \"piece\"\n"
                                         "(\n"
                                         "    \"id\"              varchar(64) primary key not null,\n"
                                         "    \"index\"           int(11)                 not null,\n"
                                         "    \"segment_id\"      varchar(64)             not null,\n"
                                         "    \"storage_node_id\" int(11)                 not null\n"
                                         ");";
    const char *sql_create_table_storage_node = "create table if not exists \"storage_node\"\n"
                                                "(\n"
                                                "    \"id\" varchar(64) primary key not null\n"
                                                ");";
    // 打开数据库
    sqlite3_open_v2("storj.db", &sql, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, nullptr);
 std::cout << "drop table !!!! \n" << std::endl;
// sqlite3_exec(sql, "drop table file;", nullptr, nullptr, nullptr);sqlite3_exec(sql, "drop table segment;", nullptr, nullptr, nullptr);sqlite3_exec(sql, "drop table piece;", nullptr, nullptr, nullptr);sqlite3_exec(sql, "drop table storage_node;", nullptr, nullptr, nullptr);

    sqlite3_exec(sql, sql_create_table_file, nullptr, nullptr, nullptr);
    sqlite3_exec(sql, sql_create_table_segment, nullptr, nullptr, nullptr);
    sqlite3_exec(sql, sql_create_table_piece, nullptr, nullptr, nullptr);

    sqlite3_exec(sql, sql_create_table_storage_node, nullptr, nullptr, nullptr);
}

void data_manager::init_storage_nodes()
{
    // 添加数据库记录
    int node_count;
    {
        const char *sql_select = "select count(1)\n"
                                 "from \"storage_node\";";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr);
        sqlite3_step(stmt);
        node_count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    // 补足节点数量
    for (int i = node_count; i < storage_node_num; i++) {
        boost::uuids::random_generator uuid_v4;
        const std::string &node_id = to_string(uuid_v4());
        const char *sql_insert = "insert into \"storage_node\"(\"id\")\n"
                                 "values (?);";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(sql, sql_insert, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, node_id.c_str(), node_id.length(), nullptr);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    // 查出所有节点，加入到 set 中
    {
        boost::uuids::string_generator sg;
        const char *sql_select = "select \"id\"\n"
                                 "from \"storage_node\";";
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const boost::uuids::uuid &node_id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 0)));
            storage_nodes.emplace(node_id);
        }
        sqlite3_finalize(stmt);
    }
    // 创建目录
    mkdir(storage_node_base_path.c_str(), 0755);
    for (const auto &node: storage_nodes) {
        mkdir(get_storage_node_path(node).c_str(), 0755);
    }
}

std::string data_manager::get_storage_node_path(const storage_node &node)
{
    return get_storage_node_path(to_string(node.id));
}

std::string data_manager::get_storage_node_path(const std::string &node_id)
{
    return storage_node_base_path + node_id;
}

std::string data_manager::get_piece_path(const std::string &node_id, const std::string &piece_id)
{
    return get_storage_node_path(node_id) + "/" + piece_id;
}

std::string data_manager::get_piece_path(const std::string &piece_id)
{
    const piece &piece = db_select_piece(piece_id);
    return get_piece_path(to_string(piece.storage_node_id), piece_id);
}

void data_manager::db_insert_file(const file &f)
{
    const std::string &file_id = to_string(f.id);
    const char *sql_insert = "insert into \"file\"(\"id\", \"file_name\", \"file_size\", \"segment_size\", \"stripe_size\", \"erasure_share_size\", \"k\", \"m\", \"n\")\n"
                             "values (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_insert, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, file_id.c_str(), file_id.length(), nullptr);
    sqlite3_bind_text(stmt, 2, f.name.c_str(), f.name.length(), nullptr);
    sqlite3_bind_int(stmt, 3, f.cfg.file_size);
    sqlite3_bind_int(stmt, 4, f.cfg.segment_size);
    sqlite3_bind_int(stmt, 5, f.cfg.stripe_size);
    sqlite3_bind_int(stmt, 6, f.cfg.erasure_share_size);
    sqlite3_bind_int(stmt, 7, f.cfg.k);
    sqlite3_bind_int(stmt, 8, f.cfg.m);
    sqlite3_bind_int(stmt, 9, f.cfg.n);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_insert_segment(const segment &s)
{
    const std::string &segment_id = to_string(s.id);
    const std::string &file_id = to_string(s.file_id);
    const char *sql_insert = "insert into \"segment\"(\"id\", \"index\", \"file_id\")\n"
                             "values (?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_insert, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, segment_id.c_str(), segment_id.length(), nullptr);
    sqlite3_bind_int(stmt, 2, s.index);
    sqlite3_bind_text(stmt, 3, file_id.c_str(), file_id.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_insert_erasure_share(const erasure_share &es)
{
    const std::string &erasure_share_id = to_string(es.id);
    const std::string &stripe_id = to_string(es.stripe_id);
    const std::string &piece_id = to_string(es.piece_id);
    const char *sql_insert = "insert into \"erasure_share\"(\"id\", \"x_index\", \"y_index\", \"stripe_id\", \"piece_id\")\n"
                             "values (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_insert, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, piece_id.c_str(), erasure_share_id.length(), nullptr);
    sqlite3_bind_int(stmt, 2, es.x_index);
    sqlite3_bind_int(stmt, 3, es.y_index);
    sqlite3_bind_text(stmt, 4, stripe_id.c_str(), stripe_id.length(), nullptr);
    sqlite3_bind_text(stmt, 5, piece_id.c_str(), piece_id.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_insert_piece(const piece &p)
{
    const std::string &piece_id = to_string(p.id);
    const std::string &segment_id = to_string(p.segment_id);
    const std::string &storage_node_id = to_string(p.storage_node_id);
    const char *sql_insert = "insert into \"piece\"(\"id\", \"index\", \"segment_id\", \"storage_node_id\")\n"
                             "values (?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_insert, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, piece_id.c_str(), piece_id.length(), nullptr);
    sqlite3_bind_int(stmt, 2, p.index);
    sqlite3_bind_text(stmt, 3, segment_id.c_str(), segment_id.length(), nullptr);
    sqlite3_bind_text(stmt, 4, storage_node_id.c_str(), storage_node_id.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_stmt_select_file(sqlite3_stmt *stmt, file *file)
{
    boost::uuids::string_generator sg;
    file->id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 0)));
    file->name = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
    file->cfg.file_size = sqlite3_column_int(stmt, 2);
    file->cfg.segment_size = sqlite3_column_int(stmt, 3);
    file->cfg.stripe_size = sqlite3_column_int(stmt, 4);
    file->cfg.erasure_share_size = sqlite3_column_int(stmt, 5);
    file->cfg.k = sqlite3_column_int(stmt, 6);
    file->cfg.m = sqlite3_column_int(stmt, 7);
    file->cfg.n = sqlite3_column_int(stmt, 8);
}

file data_manager::db_select_file_by_id(const std::string &id)
{
    file res;
    const char *sql_select = "select *\n"
                             "from \"file\"\n"
                             "where \"id\" = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
        return res;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.length(), nullptr);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        return res;
    }
    db_stmt_select_file(stmt, &res);
    sqlite3_finalize(stmt);
    return res;
}

file data_manager::db_select_file_by_name(const std::string &filename)
{
    file res;
    const char *sql_select = "select *\n"
                             "from \"file\"\n"
                             "where \"file_name\" = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
        return res;
    }
    sqlite3_bind_text(stmt, 1, filename.c_str(), filename.length(), nullptr);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        // res.id = boost::uuids::nil;
        return res;
    }
    db_stmt_select_file(stmt, &res);
    sqlite3_finalize(stmt);
    return res;
}

segment data_manager::db_select_segment(const std::string &id)
{
    boost::uuids::string_generator sg;
    segment res;
    const char *sql_select = "select *\n"
                             "from \"segment\"\n"
                             "where \"id\" = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
        return res;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.length(), nullptr);
    sqlite3_step(stmt);
    res.id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 0)));
    res.index = sqlite3_column_int(stmt, 1);
    res.file_id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 2)));
    sqlite3_finalize(stmt);
    return res;
}

piece data_manager::db_select_piece(const std::string &id)
{
    boost::uuids::string_generator sg;
    piece res;
    const char *sql_select = "select *\n"
                             "from \"piece\"\n"
                             "where \"id\" = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
        return res;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.length(), nullptr);
    sqlite3_step(stmt);
    res.id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 0)));
    res.index = sqlite3_column_int(stmt, 1);
    res.segment_id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 2)));
    res.storage_node_id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 3)));
    sqlite3_finalize(stmt);
    return res;
}

void data_manager::db_remove_file_by_id(const std::string &id)
{
    const char *sql_remove = "delete\n"
                             "from \"file\"\n"
                             "where \"id\" = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_remove, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), id.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_remove_file_by_name(const std::string &name)
{
    const char *sql_remove = "delete\n"
                             "from \"file\"\n"
                             "where \"name\" = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_remove, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(), name.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_remove_segment(const std::string &id)
{
    const char *sql_remove = "delete\n"
                             "from \"segment\"\n"
                             "where \"id\" = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_remove, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), id.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::db_remove_piece(const std::string &id)
{
    const char *sql_remove = "delete\n"
                             "from \"piece\"\n"
                             "where \"id\" = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sql, sql_remove, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, id.c_str(), id.length(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void data_manager::upload_piece(const piece &p, const storage_node &node)
{
    const std::string &piece_path = get_piece_path(to_string(node.id), to_string(p.id));
    // 创建文件
    int fd = open(piece_path.c_str(), O_CREAT | O_EXCL | O_RDWR);
    if (fd == -1) {
        perror("upload piece: Failed to open file");
        return;
    }
    // 写内容
    const int unit = 16 << 10;
    for (int i = 0; i < p.data.size(); i += unit) {
        int n = std::min(unit, (int) p.data.size() - i);
        if (write(fd, p.data.data() + i, n) <= 0) {
            perror("upload piece: Failed to write file");
            return;
        }
    }
    // 关闭文件
    close(fd);
}

piece data_manager::download_piece(const std::string &piece_id)
{
    piece piece = db_select_piece(piece_id);
    const std::string &piece_path = get_piece_path(to_string(piece.storage_node_id), to_string(piece.id));
    // 创建文件
    int fd = open(piece_path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("download piece: Failed to open file");
        return piece;
    }
    // 读内容
    const int unit = 16 << 10;
    char buf[unit];
    int i = 0;
    int n;
    while ((n = read(fd, buf, unit)) > 0) {
        piece.data.insert(piece.data.end(), buf, buf + n);
        i += n;
    }
    // 关闭文件
    close(fd);
    return piece;
}

void data_manager::remove_piece(const std::string &piece_id)
{
    const std::string &path = get_piece_path(piece_id);
    if (remove(path.c_str()) == -1) {
        perror("remove piece: Failed to remove piece file");
        db_remove_piece(piece_id);
        return;
    }
    db_remove_piece(piece_id);
}

bool data_manager::audit_piece(const std::string &piece_id)
{
    piece piece = db_select_piece(piece_id);
    const std::string &piece_path = get_piece_path(to_string(piece.storage_node_id), to_string(piece.id));
    int fd = open(piece_path.c_str(), O_RDONLY);
    if (fd <= 0) {
        return false;
    }
    close(fd);
    return true;
}

/**
 * 以指定 (k, m, n) 等配置上传指定文件
 * <ol>
 * <li> 读取 file 内容，切割成 segments
 * <li> 遍历 segments，切割成 stripes
 * <li> 遍历 stripes，纠删编码为 erasure shares
 * <li> 组合 erasure shares，拼接成 pieces
 * <li> pieces 分发到各个 storage nodes
 * </ol>
 * @param filename 文件名
 * @param cfg 配置
 */
void data_manager::upload_file(const std::string &filename, config &cfg)
{
    
 
    // 判断是否有同名文件
    std::cout << filename << std::endl;
    const file &record = db_select_file_by_name(filename);
    std::cout << record.id << std::endl;
    std::cout << record.name << std::endl;
    if (record.name == filename) {
        puts("存在同名文件，无法上传");
        return;
    }

    try {
        // 开始数据库事务
        sqlite3_exec(sql, "begin transaction;", nullptr, nullptr, nullptr);

        data_processor dp(cfg);

        // 随机生成 ID，记录数据对应关系到数据库
        boost::uuids::random_generator uuid_v4;
        file file(filename, cfg);
        file.id = uuid_v4();
        db_insert_file(file);

        // 读文件，切割成 segment 并遍历
        std::vector<segment> segments = dp.split_file(file);
        for (int segment_index = 0; segment_index < segments.size(); segment_index++) {
            //printf("segment index: %d\n", segment_index);
            segment &segment = segments[segment_index];
            // segment id
            segment.id = uuid_v4();
            segment.index = segment_index;
            segment.file_id = file.id;
            db_insert_segment(segment);
            // 切割成 stripes 并遍历
            //puts("split segment");
            std::vector<stripe> stripes = dp.split_segment(segment);
            std::vector<std::vector<erasure_share>> s;
            s.reserve(stripes.size());
            for (auto &stripe: stripes) {
                // 编码成 erasure shares，该数组为纵向
                std::vector<erasure_share> shares = dp.erasure_encode(stripe);
                s.emplace_back(shares);
            }

            // erasure shares 横向合并成 pieces
            //puts("merge to pieces");
            std::vector<piece> pieces = dp.merge_to_pieces(s);
            for (int piece_index = 0; piece_index < pieces.size(); piece_index++) {
                piece &piece = pieces[piece_index];
                // piece id
                piece.id = uuid_v4();
                piece.index = piece_index;
                piece.segment_id = segment.id;
                for (auto &share: piece.erasure_shares) {
                    share.piece_id = piece.id;
                }
            }

            // 上传 pieces 到各个存储节点
            auto piece = pieces.begin();
            auto storage_node = storage_nodes.begin();
            while (piece != pieces.end()) {
                piece->storage_node_id = storage_node->id;
                upload_piece(*piece, *storage_node);
                //printf("upload piece id: %s\n", to_string(piece->id).c_str());
                db_insert_piece(*piece);
                piece++;
                storage_node++;
                // 遍历到最后一个存储节点后，从第一个重新开始遍历
                if (storage_node == storage_nodes.end()) {
                    storage_node = storage_nodes.begin();
                }
            }
        }
    } catch (int e) {
        // 出现异常，回滚数据库
        perror("Failed to upload file");
        sqlite3_exec(sql, "rollback;", nullptr, nullptr, nullptr);
        return;
    }
    // 提交事务
    sqlite3_exec(sql, "commit;", nullptr, nullptr, nullptr);
    puts("Upload file: Commit!!");

 }


/**
 * 下载指定文件
 * <ol>
 * <li> 读取数据库，得到 file 元数据
 * <li> 查询 file 对应的 segments
 * <li> 以 segment 为单位，查询对应的 pieces 元数据
 * <li> 从相应的 storage nodes 下载得到 pieces
 * <li> 解析 pieces 为 erasure shares
 * <li> erasure shares 纠删解码成 stripes
 * <li> stripes 拼接成 segments
 * <li> segments 拼接成 file
 * </ol>
 * @param filename 文件名
 * @return 文件
 */
file data_manager::download_file(const std::string &filename)
{   
    
    // 从数据库中查出对应的 file 数据
    file file = db_select_file_by_name(filename);
    data_processor dp(file.cfg);

    // 从数据库中有序查出对应的 piece 数据
    // 建立 segment id 到 pieces 的映射
    boost::uuids::string_generator sg;
    std::vector<std::pair<std::string, std::vector<piece>>> segment_id_to_pieces;
    {
        const char *sql_select = "select \"p\".\"id\",\n"
                                 "       \"sn\".\"id\",\n"
                                 "       \"s\".\"id\"\n"
                                 "from \"file\" \"f\"\n"
                                 "         left join \"segment\" \"s\" on \"f\".\"id\" = \"s\".\"file_id\"\n"
                                 "         left join \"piece\" \"p\" on \"s\".\"id\" = \"p\".\"segment_id\"\n"
                                 "         left join \"storage_node\" \"sn\" on \"sn\".\"id\" = \"p\".\"storage_node_id\"\n"
                                 "where \"f\".\"file_name\" = ?\n"
                                 "order by \"s\".\"index\", \"p\".\"index\";";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
            return file;
        }
        sqlite3_bind_text(stmt, 1, filename.c_str(), filename.length(), nullptr);
        std::vector<piece> pieces;
        std::string last_segment_id;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            piece p;
            p.id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 0)));
            p.storage_node_id = sg(reinterpret_cast<const char *const>(sqlite3_column_text(stmt, 1)));
            p.segment_id = sg(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
            pieces.push_back(p);
            const std::string &segment_id = to_string(p.segment_id);
            if (last_segment_id != segment_id) {
                last_segment_id = segment_id;
                segment_id_to_pieces.emplace_back(segment_id, std::vector<piece>(0));
            }
            segment_id_to_pieces.back().second.push_back(p);
        }
        sqlite3_finalize(stmt);
    }
    printf("segment num: %d\n", segment_id_to_pieces.size());

    // 遍历映射表，以 segment 为单位处理 piece
    std::vector<segment> segments;
    for (auto &pair: segment_id_to_pieces) {
        const std::string &segment_id = pair.first;
        std::vector<piece> &pieces = pair.second;
        // 从相应的 storage node 下载 piece data
        for (auto &piece: pieces) {
            const std::string &piece_id = to_string(piece.id);
            //printf("download piece id: %s\n", piece_id.c_str());
            piece.data = std::move(download_piece(piece_id).data);
        }

        // 遍历 pieces
        // 由于数据库查询 piece 时已经排序，此二维数组 erasure share 有序
        puts("split piece");
        std::vector<std::vector<erasure_share>> s;
        for (auto &piece: pieces) {
            // piece 拆分成 erasure share（横向）
            std::vector<erasure_share> shares = dp.split_piece(piece);
		//std::vector<erasure_share> shares;
            s.emplace_back(shares);
        }

        puts("merge to stripes");
        // erasure share decode，纵向拼接成 stripe，赋值元数据
        std::vector<stripe> stripes = dp.merge_to_stripes(s);
        //std::cout << stripes.size() << std::endl;
        //for (const auto c : stripes[0].data) std::cout << c << " ";
        //std::cout << std::endl;
        //puts("merge to segment");
        // stripe 拼接成 segment，查询出 segment 元数据
        segment segment = db_select_segment(segment_id);
        segment.data = std::move(dp.merge_to_segment(stripes).data);
        //for (const auto c : segment.data) std::cout << c << " ";
        //std::cout << std::endl;
        segments.emplace_back(segment);
    }

    //puts("merge to file");
    // segment 拼接成 file
    //std::cout << segments.size() << std::endl;
    //for (const auto c : segments[0].data) std::cout << c << " ";
        //std::cout << std::endl;
    file.segments = segments;
    
    return file;
}

/**
 * 扫描需要修复的 segments
 * @return segment ids, ks, rs
 */
std::tuple<std::vector<std::string>, std::vector<int>, std::vector<int>, std::unordered_map<std::string, int>> data_manager::scan_corrupted_segments()
{
    std::cout << "begin scan" << std::endl;
    std::vector<std::string> segments_to_repair;
    std::vector<int> ks;
    std::vector<int> rs;
    std::unordered_map<std::string, int> file_corrupted_segment_size;
    

    // 查询并遍历所有 file
    std::vector<file> files;
    {
        const char *sql_select = "select *\n"
                                 "from \"file\";";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
            return {};
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            file file;
            db_stmt_select_file(stmt, &file);
            files.emplace_back(file);
        }
        sqlite3_finalize(stmt);
    }
    std::cout << "file size :" << files.size() << std::endl;
    for (const auto &file: files) {
	int now_file_segments_corrupted_size = 0;
        // 查询每个 file 对应的 segments
        std::vector<std::string> segment_ids;
        {
            const char *sql_select = "select \"p\".\"id\"\n"
                                     "from \"file\" \"s\"\n"
                                     "         left join \"segment\" \"p\" on \"s\".\"id\" = \"p\".\"file_id\"\n"
                                     "where \"s\".\"id\" = ? \n"
                                     "order by \"p\".\"index\";";
            sqlite3_stmt *stmt;
            const std::string &file_id = to_string(file.id);
            // std::cout << boost::uuids::to_string(file.id).c_str() << " "<< to_string(file.id).length()<< std::endl;
            if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
                continue;
            }
            sqlite3_bind_text(stmt, 1, file_id.c_str(), file_id.length(), nullptr);
            // std::cout << sqlite3_expanded_sql(stmt) << std::endl;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const std::string &segment_id = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
                segment_ids.emplace_back(segment_id);
            }
            sqlite3_finalize(stmt);
        }
        // 以 segment 为单位，查询所有对应的 piece
        std::cout << "segment size : " << segment_ids.size() << std::endl;
        for (const auto &segment_id: segment_ids) {
            std::vector<std::string> piece_ids;
            const char *sql_select = "select \"p\".\"id\"\n"
                                     "from \"segment\" \"s\"\n"
                                     "         left join \"piece\" \"p\" on \"s\".\"id\" = \"p\".\"segment_id\"\n"
                                     "where \"s\".\"id\" = ?;";
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
                continue;
            }
            sqlite3_bind_text(stmt, 1, segment_id.c_str(), segment_id.length(), nullptr);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                piece_ids.emplace_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))));
            }
            sqlite3_finalize(stmt);
            // 审计 piece
            // std::cout << piece_ids.size() << std::endl;
            int count = 0;
            for (const auto &piece_id: piece_ids) {
                if (audit_piece(piece_id)) {
                    count++;
                }
            }
           // std::cout << "count size " << count << std::endl;
            // 当剩余 piece 数小于 m 时，将 segment id 和缺失的 piece id 加入到结果中
            // k = 5 , m = 2 n = 7
            if (count < file.cfg.n){
 		now_file_segments_corrupted_size ++;
                segments_to_repair.emplace_back(segment_id);
                ks.emplace_back(file.cfg.k);
                rs.emplace_back(count);
            }
        }
	file_corrupted_segment_size.emplace(to_string(file.id), now_file_segments_corrupted_size);
    }
    return std::make_tuple(segments_to_repair, ks, rs, file_corrupted_segment_size);
}

/**
 * 以 segment 为单位修复，步骤：
 * <ol>
 * <li> 查询对应的 file 配置 (k, m, n)
 * <li> 查询对应的 pieces
 * <li> 下载剩余的 pieces
 * <li> pieces 解析为 erasure shares
 * <li> erasure shares 恢复成 stripes
 * <li> stripes 重新纠删编码成 erasure shares
 * <li> erasure shares 合并成 pieces
 * <li> pieces 分发到各个 storage nodes
 * </ol>
 * @param segment_id
 */
void data_manager::repair_segment(const std::string &segment_id)
{    long total_repair = 0;
    long duration1 = 0;
long duration2 = 0;
long duration3 = 0;
long duration4 = 0;
    try {
        // 开始数据库事务
        sqlite3_exec(sql, "begin transaction;", nullptr, nullptr, nullptr);

        // 查询对应的文件配置
        const segment &segment = db_select_segment(segment_id);
        const file &file = db_select_file_by_id(to_string(segment.file_id));
        data_processor dp(file.cfg);
        boost::uuids::random_generator uuid_v4;
        boost::uuids::string_generator sg;
        // 有序查询所有对应的 piece
        std::vector<std::string> piece_ids;
        {
            const char *sql_select = "select \"p\".\"id\"\n"
                                     "from \"segment\" \"s\"\n"
                                     "         left join \"piece\" \"p\" on \"s\".\"id\" = \"p\".\"segment_id\"\n"
                                     "where \"s\".\"id\" = ?\n"
                                     "order by \"p\".\"index\";";
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(sql, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
                return;
            }
            sqlite3_bind_text(stmt, 1, segment_id.c_str(), segment_id.length(), nullptr);
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                piece_ids.emplace_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))));
            }
            sqlite3_finalize(stmt);
        }

        // 下载剩余的 pieces
        std::vector<std::vector<erasure_share>> s;
        std::vector<piece> pieces;
	// double totol
	// t1 t2
	long t1, t2;
	
        for (const auto &piece_id: piece_ids) {
            piece piece = download_piece(piece_id);
            // 跳过无效 piece
            if (piece.id.is_nil()) {
                continue;
            }
            pieces.emplace_back(piece);
            // piece 拆分成 erasure share（横向）
	    // clock() t1
	    t1 = gettimens();
            std::vector<erasure_share> shares = dp.split_piece(piece);
            s.emplace_back(shares);
	    // clock() t2
	    t2 = gettimens();
	    duration1+= t2-t1;
	    // total += t2-t2
        }
	// part1 !!! log
	std::ofstream mycout("test_data.txt",std::ios::app);

	mycout << "new Segment !!!!!! " << segment_id  << std::endl;
	mycout << "file_size: " << file.cfg.file_size << " bytes, segment_size : "<<file.cfg.segment_size << " bytes, stripe_size: " << file.cfg.stripe_size << " byte, k: " << file.cfg.k << ", m :" <<file. cfg.m << ", n :" << file.cfg.n << std::endl;
	mycout<<"Part1 piece to erasure:"<<duration1<<std::endl;
	total_repair+=duration1;
//	std::cout << "part 1 !" << du << std::endl;
	long t3, t4;

        // erasure shares 恢复成 stripes，计算并修复数据
        // std::vector<stripe> stripes = dp.repair_stripes_from_erasure_shares(s);
       // clock_t  start,stop;
	t3 = gettimens();
	// t1 -- decoder
	std::vector<stripe> stripes = dp.merge_to_stripes(s);
	t4 = gettimens();
	duration2 =(t4-t3);
      // std::ofstream mycout("test_data.txt",std::ios::app);
      mycout<<"Part2 erasure to stripe: "<<duration2<<std::endl;
      mycout << "decode  thoughput for one segment  is : " << (double)( (file.cfg.segment_size / 1024.0 / 1024.0)  * 1000000000.0) / duration2 << " MB / s" << std::endl; 

total_repair+=duration2;

//	std::cout << "part 2" << du << std::endl;
	/// log 2 !!!!!!!
        // t2 
	// total += t2 - t1 (part 2)
	s.clear();
     //   stop = clock();
//	duration = ((double)(stop-start))/CLOCK_TAI;
//	std::ofstream mycout("test_data.txt",std::ios::app);
//	mycout<<"Stripe Repair time:"<<duration<<std::endl;
        // 新 stripes 处理成 pieces
        s.reserve(stripes.size());
	// decode 部分 total_2
	// t1
	long t5, t6;
	long t5_t6_total = 0;

        for (auto &stripe: stripes) {
	    t5 = gettimens();
            // 编码成 erasure shares，该数组为纵向
	    // t11
            std::vector<erasure_share> shares = dp.erasure_encode(stripe);
            // t22 单独记录
	    s.emplace_back(shares);
	    t6 = gettimens();
	    duration3 += t6 - t5;
	    t5_t6_total += t6 - t5;
	    // mycout << "Stripe encode"<<t6 - t5<<std::endl;
	   // !!! log self
        }
	mycout << "Part3 Inner log encode :"<<  duration3 << std::endl;
        mycout << "encode  thoughput for one segment  is : " << (double)( (file.cfg.segment_size / 1024.0 / 1024.0)  * 1000000000.0) / duration3 << " MB / s" << std::endl; 
	 mycout << "encode  thoughput for one stripe agv  is : " << (double)( (file.cfg.segment_size / 1024.0 / 1024.0)  * 1000000000.0) / t5_t6_total << " MB / s" << std::endl; 
 


	total_repair+=duration3;

	//std::cout << "part 3 " << du << std::endl;
	// log !!!! du
	//
	long t7,t8;
	// t2
	// total_2 += t2 - t1 (part 3)
	
        // erasure shares 横向合并成 pieces
	//
	// total_3
	// t1
	t7 = gettimens();
        std::vector<piece> pieces_new = dp.merge_to_pieces(s);
        // t2
	t8 = gettimens();
	// total_3 += t2 -t1
        duration4 = t8 - t7;
	mycout << "Part 4 Merge to piece :" << duration4 << std::endl;
        total_repair+=duration4;	
	mycout << "Total segment repair time :" << total_repair << std::endl;
	mycout << "The repair thougout is : " << (double)((double)( (file.cfg.segment_size / 1024.0 / 1024.0)  * 1000000000.0) / total_repair) << " MB / s" << std::endl; 
	mycout.close();
	// !! log 
	for (int i = 0; i < pieces_new.size(); i++) {
            piece &piece = pieces_new[i];
            // piece id
            piece.id = uuid_v4();
            piece.index = i;
            piece.segment_id = segment.id;
        }

        // 上传 pieces 到各个存储节点
        auto piece = pieces_new.begin();
        auto storage_node = storage_nodes.begin();
        while (piece != pieces_new.end()) {
            piece->storage_node_id = storage_node->id;
            upload_piece(*piece, *storage_node);
            db_insert_piece(*piece);
            piece++;
            storage_node++;
            // 遍历到最后一个存储节点后，从第一个重新开始遍历
            if (storage_node == storage_nodes.end()) {
                storage_node = storage_nodes.begin();
            }
        }
// stop=clock();

//     duration=((double)(stop-start))/CLOCK_TAI;

//     std::cout<<"Total repair "<<duration<<std::endl;
        // 删除数据库中的旧的 piece 和 storage nodes 中的 pieces
        for (const auto &piece_id: piece_ids) {
            remove_piece(piece_id);
        }
    } catch (int e) {
        perror("Failed to repair segment");
        sqlite3_exec(sql, "rollback;", nullptr, nullptr, nullptr);
    }
    sqlite3_exec(sql, "commit;", nullptr, nullptr, nullptr);
    puts("Repair segment: Commit");
    
}

void data_manager::sort_segments(std::vector<std::string> &segment_ids, std::vector<int> &ks, std::vector<int> &rs)
{
    if (segment_ids.size() != ks.size() || segment_ids.size() != rs.size()) {
        return;
    }
    // 记录 segment id 与 k, r 的对应关系
    std::unordered_map<std::string, std::pair<int, int>> map;
    for (int i = 0; i < segment_ids.size(); i++) {
        map.emplace(segment_ids[i], std::make_pair(ks[i], rs[i]));
    }

    // 评分函数
    const auto &weight = [=](const int k, const int r) {
        double churn_per_round = config::failure_rate * config::total_nodes;
        if (churn_per_round < config::min_churn_per_round) {
            churn_per_round = config::min_churn_per_round;
        }
        double p = double(config::total_nodes - r) / config::total_nodes;
        double mean = double(r - k + 1) * p / (1 - p);
        return mean / churn_per_round;
    };

    // 排序规则
    const auto &less = [=](const std::string &a, const std::string &b) {
        const std::pair<int, int> &p1 = map.at(a);
        const std::pair<int, int> &p2 = map.at(b);
        return weight(p1.first, p1.second) < weight(p2.first, p2.second);
    };

    // 排序
    std::sort(segment_ids.begin(), segment_ids.end(), less);

    // 重新赋值 ks, rs
    for (int i = 0; i < segment_ids.size(); i++) {
        const auto &pair = map[segment_ids[i]];
        ks[i] = pair.first;
        rs[i] = pair.second;
    }
}
