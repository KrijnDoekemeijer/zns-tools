#include "f2fs_fs.h"

struct f2fs_super_block f2fs_sb;
struct f2fs_checkpoint f2fs_cp;

/*
 * Read a block of specified size from the device
 *
 * @fd: open file descriptor to the device containg the block
 * @dest: void * to the destination buffer
 * @offset: starting offset to read from
 * @size: size in bytes to read
 *
 * returns: 1 on success, 0 on Failure
 *
 * */
static int f2fs_read_block(int fd, void *dest, __u64 offset, size_t size) {
    if (lseek(fd, offset, SEEK_SET) < 0) {
        return 0;
    }

    if (read(fd, dest, size) < 0) {
        return 0;
    }

    return 1;
}

/*
 * Read the superblock from the provided device
 *
 * @dev_path: char * to the device containing the superblock
 *
 * Note, function sets the global f2fs_sb struct with the read superblock data.
 *
 * */
void f2fs_read_super_block(char *dev_path) {
    int fd;

    fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        ERR_MSG("opening device fd for %s\n", dev_path);
    }

    if (!f2fs_read_block(fd, &f2fs_sb, F2FS_SUPER_OFFSET,
                         sizeof(struct f2fs_super_block))) {
        ERR_MSG("reading superblock from %s\n", dev_path);
    }

    close(fd);
}

/* 
 * Print all information in the superblock
 *
 * */
void show_super_block() {

    MSG("================================================================"
        "=\n");
    MSG("\t\t\tSUPER BLOCK\n");
    MSG("================================================================"
        "=\n");
    MSG("magic: \t\t\t%#10"PRIx32"\n", f2fs_sb.magic); 
    MSG("major_version: \t\t%hu\n", f2fs_sb.major_ver); 
    MSG("minor_version: \t\t%hu\n", f2fs_sb.minor_ver); 
    MSG("log_sectorsize: \t%u\n", f2fs_sb.log_sectorsize);
    MSG("log_sectors_per_block: \t%u\n", f2fs_sb.log_sectors_per_block);
    MSG("log_blocksize: \t\t%u\n", f2fs_sb.log_blocksize);
    MSG("log_blocks_per_seg: \t%u\n", f2fs_sb.log_blocks_per_seg);
    MSG("segs_per_sec: \t\t%u\n", f2fs_sb.segs_per_sec);
    MSG("secs_per_zone: \t\t%u\n", f2fs_sb.secs_per_zone);
    MSG("checksum_offset: \t%u\n", f2fs_sb.checksum_offset);
    MSG("block_count: \t\t%llu\n", f2fs_sb.block_count);
    MSG("section_count: \t\t%u\n", f2fs_sb.section_count);
    MSG("segment_count: \t\t%u\n", f2fs_sb.segment_count);
    MSG("segment_count_ckpt: \t%u\n", f2fs_sb.segment_count_ckpt);
    MSG("segment_count_sit: \t%u\n", f2fs_sb.segment_count_sit);
    MSG("segment_count_nat: \t%u\n", f2fs_sb.segment_count_nat);
    MSG("segment_count_ssa: \t%u\n", f2fs_sb.segment_count_ssa);
    MSG("segment_count_main: \t%u\n", f2fs_sb.segment_count_main);
    MSG("segment0_blkaddr: \t%u\n", f2fs_sb.segment0_blkaddr);
    MSG("cp_blkaddr: \t\t%#"PRIx32"\n", f2fs_sb.cp_blkaddr);
    MSG("sit_blkaddr: \t\t%#"PRIx32"\n", f2fs_sb.sit_blkaddr);
    MSG("nat_blkaddr: \t\t%#"PRIx32"\n", f2fs_sb.nat_blkaddr);
    MSG("ssa_blkaddr: \t\t%#"PRIx32"\n", f2fs_sb.ssa_blkaddr);
    MSG("main_blkaddr: \t\t%#"PRIx32"\n", f2fs_sb.main_blkaddr);
    MSG("root_ino: \t\t%u\n", f2fs_sb.root_ino);
    MSG("node_ino: \t\t%u\n", f2fs_sb.node_ino);
    MSG("meta_ino: \t\t%u\n", f2fs_sb.meta_ino);
    MSG("extension_count: \t%u\n", f2fs_sb.extension_count);

    MSG("Extensions: \t\t");
    for (uint8_t i = 0; i < F2FS_MAX_EXTENSION; i++) {
        MSG("%s ", f2fs_sb.extension_list[i]);
    } 
    MSG("\n");

    MSG("cp_payload: \t\t%u\n", f2fs_sb.cp_payload);
    MSG("version: \t\t%s\n", f2fs_sb.version);
    MSG("init_version: \t\t%s\n", f2fs_sb.init_version);
    MSG("feature: \t\t%u\n", f2fs_sb.feature);
    MSG("encryption_level: \t%u\n", f2fs_sb.encryption_level);
    MSG("encrypt_pw_salt: \t\t%s\n", f2fs_sb.encrypt_pw_salt);

    MSG("Devices: \t\t");
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        if (f2fs_sb.devs[i].total_segments == 0) {
            MSG("\n");
            break;
        }
        MSG("%s ", f2fs_sb.devs[i].path);
    } 

    MSG("hot_ext_count: \t\t%hhu\n", f2fs_sb.hot_ext_count);
    MSG("s_encoding: \t\t%hu\n", f2fs_sb.s_encoding);
    MSG("s_encoding_flags: \t%hu\n", f2fs_sb.s_encoding_flags);
    MSG("crc: \t\t\t%u\n", f2fs_sb.crc);
}

void f2fs_read_checkpoint(char *dev_path) {
    int fd;

    fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        ERR_MSG("opening device fd for %s\n", dev_path);
    }

    if (!f2fs_read_block(fd, &f2fs_cp, f2fs_sb.cp_blkaddr << F2FS_BLKSIZE_BITS,
                         sizeof(struct f2fs_checkpoint))) {
        ERR_MSG("reading checkpoint from %s\n", dev_path);
    }

    close(fd);
}

void show_checkpoint() {
    MSG("================================================================"
        "=\n");
    MSG("\t\t\tCHECKPOINT\n");
    MSG("================================================================"
        "=\n");

    MSG("checkpoint_ver: \t\t%llu\n", f2fs_cp.checkpoint_ver);
    MSG("user_block_count: \t\t%llu\n", f2fs_cp.user_block_count);
    MSG("valid_block_count: \t\t%llu\n", f2fs_cp.valid_block_count);
    MSG("rsvd_segment_count: \t\t%u\n", f2fs_cp.rsvd_segment_count);
    MSG("overprov_segment_count: \t%u\n", f2fs_cp.overprov_segment_count);
    MSG("free_segment_count: \t\t%u\n", f2fs_cp.free_segment_count);

    MSG("ckpt_flags: \t\t\t%u\n", f2fs_cp.ckpt_flags);
    MSG("cp_pack_total_block_count: \t%u\n", f2fs_cp.cp_pack_total_block_count);
    MSG("cp_pack_start_sum: \t\t%u\n", f2fs_cp.cp_pack_start_sum);
    MSG("valid_node_count: \t\t%u\n", f2fs_cp.valid_node_count);
    MSG("valid_inode_count: \t\t%u\n", f2fs_cp.valid_inode_count);
    MSG("next_free_nid: \t\t\t%u\n", f2fs_cp.next_free_nid);
    MSG("sit_ver_bitmap_bytesize: \t%u\n", f2fs_cp.sit_ver_bitmap_bytesize);
    MSG("nat_ver_bitmap_bytesize: \t%u\n", f2fs_cp.nat_ver_bitmap_bytesize);
    MSG("checksum_offset: \t\t%u\n", f2fs_cp.checksum_offset);
    MSG("elapsed_time: \t\t\t%llu\n", f2fs_cp.elapsed_time);
    MSG("alloc_type[CURSEG_HOT_NODE]: \t%hhu\n", f2fs_cp.alloc_type[CURSEG_HOT_NODE]);
    MSG("alloc_type[CURSEG_WARM_NODE]: \t%hhu\n", f2fs_cp.alloc_type[CURSEG_WARM_NODE]);
    MSG("alloc_type[CURSEG_COLD_NODE]: \t%hhu\n", f2fs_cp.alloc_type[CURSEG_COLD_NODE]);
    MSG("cur_node_segno[0]: \t\t%u\n", f2fs_cp.cur_node_segno[0]);
    MSG("cur_node_segno[1]: \t\t%u\n", f2fs_cp.cur_node_segno[1]);
    MSG("cur_node_segno[2]: \t\t%u\n", f2fs_cp.cur_node_segno[2]);

    MSG("cur_node_blkoff[0]: \t\t%u\n", f2fs_cp.cur_node_blkoff[0]);
    MSG("cur_node_blkoff[1]: \t\t%u\n", f2fs_cp.cur_node_blkoff[1]);
    MSG("cur_node_blkoff[2]: \t\t%u\n", f2fs_cp.cur_node_blkoff[2]);

    MSG("alloc_type[CURSEG_HOT_DATA]: \t%hhu\n", f2fs_cp.alloc_type[CURSEG_HOT_DATA]);
    MSG("alloc_type[CURSEG_WARM_DATA]: \t%hhu\n", f2fs_cp.alloc_type[CURSEG_WARM_DATA]);
    MSG("alloc_type[CURSEG_COLD_DATA]: \t%hhu\n", f2fs_cp.alloc_type[CURSEG_COLD_DATA]);

    MSG("cur_data_segno[0]: \t\t%u\n", f2fs_cp.cur_data_segno[0]);
    MSG("cur_data_segno[1]: \t\t%u\n", f2fs_cp.cur_data_segno[1]);
    MSG("cur_data_segno[2]: \t\t%u\n", f2fs_cp.cur_data_segno[2]);

    MSG("cur_data_blkoff[0]: \t\t%u\n", f2fs_cp.cur_data_blkoff[0]);
    MSG("cur_data_blkoff[1]: \t\t%u\n", f2fs_cp.cur_data_blkoff[1]);
    MSG("cur_data_blkoff[2]: \t\t%u\n", f2fs_cp.cur_data_blkoff[2]);
}
