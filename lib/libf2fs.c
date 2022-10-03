#include "f2fs_fs.h"
#include <stdint.h>
#include <string.h>

struct f2fs_super_block f2fs_sb;
struct f2fs_checkpoint f2fs_cp;
uint32_t nat_block_offset = 0; /* tracking the nat block traversal */
uint32_t nat_entry_offset = 0; /* tracking offset to start at in nat_blocks */

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
void f2fs_show_super_block() {

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

void f2fs_show_checkpoint() {
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

struct f2fs_nat_entry * f2fs_get_inode_nat_entry(char *dev_path, uint32_t inode_number) {
    int fd;
    uint32_t nat_segments = 0;
    uint32_t nat_blocks = 0;
    uint64_t cur_nat_blkaddress;
    struct f2fs_nat_block *nat_block = NULL;
    struct f2fs_nat_entry *nat_entry = NULL;

    /* from:f2fs.tools mount.c:1680
     * segment_count_nat includes pair segment so divide to 2. */
    nat_segments = f2fs_sb.segment_count_nat >> 1;
    nat_blocks = nat_segments << f2fs_sb.log_blocks_per_seg;

    nat_block = (struct f2fs_nat_block *) calloc(BLOCK_SZ, 1);
    nat_entry = (struct f2fs_nat_entry *) calloc(sizeof(struct f2fs_nat_entry), 1);

    fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        ERR_MSG("opening device fd for %s\n", dev_path);
    }

    for (uint32_t i = 0; i < nat_blocks; i++) {
        cur_nat_blkaddress = (f2fs_sb.nat_blkaddr << F2FS_BLKSIZE_BITS) + (nat_block_offset * BLOCK_SZ);

        if (!f2fs_read_block(fd, nat_block, cur_nat_blkaddress,
                    BLOCK_SZ)) {
            ERR_MSG("reading NAT Block %#" PRIx64" from %s\n", cur_nat_blkaddress, dev_path);
        }

        for (uint32_t i = nat_entry_offset; i < NAT_ENTRY_PER_BLOCK; i++) {
            if (nat_block->entries[i].ino == inode_number) {
                // Next time we check nat entries, we need to start at differnet locations
                if (i == NAT_ENTRY_PER_BLOCK - 1) {
                    /* If we are at the last entry in the nat block, start at the next block */
                    nat_block_offset++;
                    nat_entry_offset = 0;
                } else {
                    /* if we are in the nat block, start at the next nat entry in this block */
                    nat_entry_offset = i + 1;
                }
                nat_entry->version = nat_block->entries[i].version;
                nat_entry->ino = nat_block->entries[i].ino;
                nat_entry->block_addr = nat_block->entries[i].block_addr;

                close(fd);
                free(nat_block);

                return nat_entry;
            }
        }
        nat_block_offset++;
        nat_entry_offset = 0;
    } 

    close(fd);
    free(nat_block);

    free(nat_entry);

    return NULL;
}

struct f2fs_node * f2fs_get_node_block(char *dev_path, uint32_t block_addr) {
    struct f2fs_node *node_block = NULL;
    int fd;

    node_block = (struct f2fs_node *) calloc(sizeof(struct f2fs_node), 1);

    fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        ERR_MSG("opening device fd for %s\n", dev_path);
    }

    if (!f2fs_read_block(fd, node_block, block_addr << F2FS_BLKSIZE_BITS,
                sizeof(struct f2fs_node))) {
        ERR_MSG("reading NAT Block %#" PRIx32" from %s\n", block_addr, dev_path);
    }

    close(fd);

    return node_block;
}

void f2fs_show_inode_info(struct f2fs_inode *inode) {
    // TODO: show flags in text and see what else we need
    MSG("i_mode: \t\t%hu\n", inode->i_mode);
    MSG("i_advise: \t\t%hu\n", inode->i_advise);
    MSG("i_inline: \t\t%hu\n", inode->i_inline);
    MSG("i_uid: \t\t\t%u\n", inode->i_uid);
    MSG("i_gid: \t\t\t%u\n", inode->i_gid);
    MSG("i_links: \t\t%u\n", inode->i_links);
    MSG("i_size: \t\t%llu\n", inode->i_size);
    MSG("i_blocks: \t\t%llu\n", inode->i_blocks);
    MSG("i_atime: \t\t%llu\n", inode->i_atime);
    MSG("i_ctime: \t\t%llu\n", inode->i_ctime);
    MSG("i_mtime: \t\t%llu\n", inode->i_mtime);
    MSG("i_atime_nsec: \t\t%u\n", inode->i_atime_nsec);
    MSG("i_ctime_nsec: \t\t%u\n", inode->i_ctime_nsec);
    MSG("i_mtime_nsec: \t\t%u\n", inode->i_mtime_nsec);
    MSG("i_generation: \t\t%u\n", inode->i_generation);
    MSG("i_xattr_nid: \t\t%u\n", inode->i_xattr_nid);
    MSG("i_flags: \t\t%u\n", inode->i_flags);
    MSG("i_pino: \t\t%u\n", inode->i_pino);
    MSG("i_namelen: \t\t%u\n", inode->i_namelen);
    MSG("i_name: \t\t%s\n", inode->i_name);
    MSG("i_dir_level: \t\t%u\n", inode->i_dir_level);
    MSG("i_nid[0] (direct): \t%u\n", inode->i_nid[0]); /* direct */
    MSG("i_nid[1] (direct): \t%u\n", inode->i_nid[1]); /* direct */
    MSG("i_nid[2] (indirect): \t%u\n", inode->i_nid[2]); /* indirect */
    MSG("i_nid[3] (indirect): \t%u\n", inode->i_nid[3]); /* indirect */
    MSG("i_nid[4] (2x indirect): %u\n", inode->i_nid[4]); /* double indirect */
}
