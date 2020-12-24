//
// Simple FIle System
// Year : 2020
// Subject : OS
// Student Name : Jang ChangHun
// Student Number :	B511169
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/* optional */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/***********/

#include "sfs_types.h"
#include "sfs_func.h"
#include "sfs_disk.h"
#include "sfs.h"

void dump_directory();

/* BIT operation Macros */
/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

static struct sfs_super spb;	// superblock
static struct sfs_dir sd_cwd = { SFS_NOINO }; // current working directory

void error_message(const char *message, const char *path, int error_code) {
	switch (error_code) {
	case -1:
		printf("%s: %s: No such file or directory\n",message, path); return;
	case -2:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -3:
		printf("%s: %s: Directory full\n",message, path); return;
	case -4:
		printf("%s: %s: No block available\n",message, path); return;
	case -5:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -6:
		printf("%s: %s: Already exists\n",message, path); return;
	case -7:
		printf("%s: %s: Directory not empty\n",message, path); return;
	case -8:
		printf("%s: %s: Invalid argument\n",message, path); return;
	case -9:
		printf("%s: %s: Is a directory\n",message, path); return;
	case -10:
		printf("%s: %s: Is not a file\n",message, path); return;
	default:
		printf("unknown error code\n");
		return;
	}
}

void sfs_mount(const char* path)
{
	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}

	printf("Disk image: %s\n", path);

	disk_open(path);
	disk_read( &spb, SFS_SB_LOCATION );

	printf("Superblock magic: %x\n", spb.sp_magic);

	assert( spb.sp_magic == SFS_MAGIC );
	
	printf("Number of blocks: %d\n", spb.sp_nblocks);
	printf("Volume name: %s\n", spb.sp_volname);
	printf("%s, mounted\n", spb.sp_volname);
	
	sd_cwd.sfd_ino = 1;		//init at root
	sd_cwd.sfd_name[0] = '/';
	sd_cwd.sfd_name[1] = '\0';
}

void sfs_umount() {

	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}
}

void sfs_mkdir(const char* org_path)
{
	int i, k, j, l, w, u;
	int newbie_ino1 = -1;
	int	newbie_ino2, newbie_ino3, touch_direct_ptr, touch_dir_entry, bitmap_block1, bitmap_block2, bitmap_block3;
	bool free_block_found = false;
	bool dir_entry_full = true;
	bool case_dir_entry_full = false;
	struct bitmap_struct {
		unsigned char bitmap[SFS_BLOCKSIZE];
	};
	struct bitmap_struct bitmap;
	struct sfs_inode si, cwd_backup, empty_node, bitmap_backup, bitmap_backup2, bitmap_backup3;
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	struct sfs_dir dir_entry_str[SFS_DENTRYPERBLOCK];
	bzero(&empty_node, SFS_BLOCKSIZE);
	disk_read(&si, sd_cwd.sfd_ino);
	disk_read(&cwd_backup, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++) {
		disk_read(dir_entry_str, si.sfi_direct[i]);
		for (w = 0; w < SFS_DENTRYPERBLOCK; w++) {
			if (dir_entry_str[w].sfd_ino > 0 && strcmp(dir_entry_str[w].sfd_name, org_path) == 0) {
				error_message("mkdir", org_path, -6);	// already exist
				return;
			}
		}
	}

	for (i = 0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0) {

			for (k = 0; k < 3; k++) {
				disk_read(&bitmap, SFS_MAP_LOCATION + k);
				for (j = 0; j < SFS_BLOCKSIZE; j++) {
					if (bitmap.bitmap[j] < 255) {
						for (u = 0; u < 8; u++) {
							if (!BIT_CHECK(bitmap.bitmap[j], u)) {
								newbie_ino1 = k * 512 * 8 + j * 8 + u;
								free_block_found = true;
								bitmap_block1 = SFS_MAP_LOCATION + k;
								BIT_SET(bitmap.bitmap[j], u);
								break;
							}
						}
						if (free_block_found) {
							break;
						}
					}
				}
				if (free_block_found) {
					break;
				}
			}

			if (!free_block_found) {
				error_message("touch", org_path, -4);	// no block available
				return;
			}

			disk_read(&bitmap_backup, bitmap_block1);
			disk_write(&bitmap, bitmap_block1);

			si.sfi_direct[i] = newbie_ino1;
			disk_write(&si, sd_cwd.sfd_ino);
			struct sfs_dir newdir[SFS_DENTRYPERBLOCK];
			bzero(&newdir, SFS_BLOCKSIZE);
			for (w = 0; w < SFS_DENTRYPERBLOCK; w++) {
				newdir[w].sfd_ino = 0;
				strncpy(newdir[w].sfd_name, "f", SFS_NAMELEN);
			}
			disk_write(&newdir, newbie_ino1);
		}

		disk_read(sd, si.sfi_direct[i]);
		for (k = 0; k < SFS_DENTRYPERBLOCK; k++) {
			if (sd[k].sfd_ino == 0) {
				dir_entry_full = false;
				break;
			}
		}
		if (!dir_entry_full) {
			touch_direct_ptr = i;
			break;
		}
		if (i == (SFS_NDIRECT - 1) && k == (SFS_DENTRYPERBLOCK - 1)) {
			error_message("touch", org_path, -3);	// directory full
			disk_write(&cwd_backup, sd_cwd.sfd_ino);
			disk_write(&empty_node, newbie_ino1);
			disk_write(&bitmap_backup, bitmap_block1);
			return;
		}
	}

	free_block_found = false;
	for (k = 0; k < 3; k++) {
		disk_read(&bitmap, SFS_MAP_LOCATION + k);
		for (j = 0; j < SFS_BLOCKSIZE; j++) {
			if (bitmap.bitmap[j] < 255) {
				for (i = 0; i < 8; i++) {
					if (!BIT_CHECK(bitmap.bitmap[j], i)) {
						newbie_ino2 = k * 512 * 8 + j * 8 + i;
						free_block_found = true;
						bitmap_block2 = SFS_MAP_LOCATION + k;
						BIT_SET(bitmap.bitmap[j], i);
						break;
					}
				}
				if (free_block_found) break;
			}
		}
		if (free_block_found) {
			break;
		}
	}

	if (!free_block_found) {
		error_message("touch", org_path, -4);	// no block available
		disk_write(&cwd_backup, sd_cwd.sfd_ino);
		disk_write(&empty_node, newbie_ino1);
		disk_write(&bitmap_backup, bitmap_block1);
		return;
	}

	disk_read(&bitmap_backup2, bitmap_block2);
	disk_write(&bitmap, bitmap_block2);

	free_block_found = false;
	for (k = 0; k < 3; k++) {
		disk_read(&bitmap, SFS_MAP_LOCATION + k);
		for (j = 0; j < SFS_BLOCKSIZE; j++) {
			if (bitmap.bitmap[j] < 255) {
				for (i = 0; i < 8; i++) {
					if (!BIT_CHECK(bitmap.bitmap[j], i)) {
						newbie_ino3 = k * 512 * 8 + j * 8 + i;
						free_block_found = true;
						bitmap_block3 = SFS_MAP_LOCATION + k;
						BIT_SET(bitmap.bitmap[j], i);
						break;
					}
				}
				if (free_block_found) break;
			}
		}
		if (free_block_found) {
			break;
		}
	}

	if (!free_block_found) {
		error_message("touch", org_path, -4);	// no block available
		disk_write(&cwd_backup, sd_cwd.sfd_ino);
		disk_write(&empty_node, newbie_ino1);
		disk_write(&bitmap_backup, bitmap_block1);
		disk_write(&bitmap_backup2, bitmap_block2);
		return;
	}

	for (l = 0; l < SFS_DENTRYPERBLOCK; l++) {
		if (sd[l].sfd_ino == 0) {
			sd[l].sfd_ino = newbie_ino2;
			strncpy(sd[l].sfd_name, org_path, SFS_NAMELEN);
			break;
		}
	}

	disk_write(sd, si.sfi_direct[touch_direct_ptr]);

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write(&si, sd_cwd.sfd_ino);

	struct sfs_inode newbie;

	bzero(&newbie, SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	newbie.sfi_size = 128;
	newbie.sfi_type = SFS_TYPE_DIR;
	newbie.sfi_direct[0] = newbie_ino3;
	disk_write(&newbie, newbie_ino2);

	struct sfs_dir newdir2[SFS_DENTRYPERBLOCK];
	bzero(&newdir2, SFS_BLOCKSIZE);
	for (w = 0; w < SFS_DENTRYPERBLOCK; w++) {
		newdir2[w].sfd_ino = 0;
		strncpy(newdir2[w].sfd_name, "f", SFS_NAMELEN);
	}
	newdir2[0].sfd_ino = newbie_ino2;
	strncpy(newdir2[0].sfd_name, ".", SFS_NAMELEN);
	newdir2[1].sfd_ino = sd_cwd.sfd_ino;
	strncpy(newdir2[1].sfd_name, "..", SFS_NAMELEN);

	disk_write(&newdir2, newbie_ino3);
	disk_write(&bitmap, bitmap_block3);
}

void sfs_touch(const char* path)
{
	//skeleton implementation
	int i, k, j, l, w, u;
	int newbie_ino1 = -1;
	int	newbie_ino2, touch_direct_ptr, touch_dir_entry, bitmap_block1, bitmap_block2;
	bool free_block_found = false;
	bool dir_entry_full = true;
	bool case_dir_entry_full = false;
	struct bitmap_struct {
		unsigned char bitmap[SFS_BLOCKSIZE];
	};
	struct bitmap_struct bitmap;
	struct sfs_inode si, cwd_backup, empty_node, bitmap_backup;
	struct sfs_dir dir_entry_str[SFS_DENTRYPERBLOCK];
	bzero(&empty_node, SFS_BLOCKSIZE);
	disk_read( &si, sd_cwd.sfd_ino );
	disk_read( &cwd_backup, sd_cwd.sfd_ino);

	//for consistency
	assert( si.sfi_type == SFS_TYPE_DIR );

	//we assume that cwd is the root directory and root directory is empty which has . and .. only
	//unused DISK2.img satisfy these assumption
	//for new directory entry(for new file), we use cwd.sfi_direct[0] and offset 2
	//becasue cwd.sfi_directory[0] is already allocated, by .(offset 0) and ..(offset 1)
	//for new inode, we use block 6 
	// block 0: superblock,	block 1:root, 	block 2:bitmap 
	// block 3:bitmap,  	block 4:bitmap 	block 5:root.sfi_direct[0] 	block 6:unused
	//
	//if used DISK2.img is used, result is not defined
	
	//buffer for disk read
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	for (i = 0; i < SFS_NDIRECT; i++) {
		disk_read(dir_entry_str, si.sfi_direct[i]);
		for (w = 0; w < SFS_DENTRYPERBLOCK; w++) {
			if (dir_entry_str[w].sfd_ino > 0 && strcmp(dir_entry_str[w].sfd_name, path) == 0) {
				error_message("touch", path, -6);	// already exist
				return;
			}
		}
	}

	//block access
	for (i = 0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0) {

			for (k = 0; k < 3; k++) {
				disk_read(&bitmap, SFS_MAP_LOCATION + k);
				for (j = 0; j < SFS_BLOCKSIZE; j++) {
					if (bitmap.bitmap[j] < 255) {
						for (u = 0; u < 8; u++) {
							if (!BIT_CHECK(bitmap.bitmap[j], u)) {
								newbie_ino1 = k * 512 * 8 + j * 8 + u;
								free_block_found = true;
								bitmap_block1 = SFS_MAP_LOCATION + k;
								BIT_SET(bitmap.bitmap[j], u);
								break;
							}
						}
						if (free_block_found) {
							break;
						}
					}
				}
				if (free_block_found) {
					break;
				}
			}

			if (!free_block_found) {
				error_message("touch", path, -4);	// no block available
				return;
			}

			disk_read(&bitmap_backup, bitmap_block1);
			disk_write(&bitmap, bitmap_block1);

			si.sfi_direct[i] = newbie_ino1;
			disk_write(&si, sd_cwd.sfd_ino);
			struct sfs_dir newdir[SFS_DENTRYPERBLOCK];
			bzero(&newdir, SFS_BLOCKSIZE);
			for (w = 0; w < SFS_DENTRYPERBLOCK; w++) {
				newdir[w].sfd_ino = 0;
				strncpy(newdir[w].sfd_name, "f", SFS_NAMELEN);
			}
			disk_write(&newdir, newbie_ino1);
		}




		disk_read(sd, si.sfi_direct[i]);
		for (k = 0; k < SFS_DENTRYPERBLOCK; k++) {
			if (sd[k].sfd_ino == 0) {
				dir_entry_full = false;
				break;
			}
		}
		if (!dir_entry_full) {
			touch_direct_ptr = i;
			break;
		}
		if (i == (SFS_NDIRECT - 1) && k == (SFS_DENTRYPERBLOCK - 1)) {
			error_message("touch", path, -3);	// directory full
			disk_write(&cwd_backup, sd_cwd.sfd_ino);
			disk_write(&empty_node, newbie_ino1);
			disk_write(&bitmap_backup, bitmap_block1);
			return;
		}
	}

	//allocate new block	
	free_block_found = false;
	for (k = 0; k < 3; k++) {
		disk_read(&bitmap, SFS_MAP_LOCATION + k);
		for (j = 0; j < SFS_BLOCKSIZE; j++) {
			if (bitmap.bitmap[j] < 255) {
				for (i = 0; i < 8; i++) {
					if (!BIT_CHECK(bitmap.bitmap[j], i)) {
						newbie_ino2 = k * 512 * 8 + j * 8 + i;
						free_block_found = true;
						bitmap_block2 = SFS_MAP_LOCATION + k;
						BIT_SET(bitmap.bitmap[j], i);
						break;
					}
				}
				if (free_block_found) break;
			}
		}
		if (free_block_found) {
			break;
		}
	}

	if (!free_block_found) {
		error_message("touch", path, -4);	// no block available
		disk_write(&cwd_backup, sd_cwd.sfd_ino);
		disk_write(&empty_node, newbie_ino1);
		disk_write(&bitmap_backup, bitmap_block1);
		return;
	}

	for (l = 0; l < SFS_DENTRYPERBLOCK; l++) {
		if (sd[l].sfd_ino == 0) {
			sd[l].sfd_ino = newbie_ino2;
			strncpy(sd[l].sfd_name, path, SFS_NAMELEN);
			break;
		}
	}

	disk_write( sd, si.sfi_direct[touch_direct_ptr] );

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );

	struct sfs_inode newbie;

	bzero(&newbie,SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	newbie.sfi_size = 0;
	newbie.sfi_type = SFS_TYPE_FILE;

	disk_write( &newbie, newbie_ino2 );
	disk_write( &bitmap, bitmap_block2);
}

void sfs_cd(const char* path)
{
	if (path != NULL) {
		int i, j;
		bool found = false;
		struct sfs_inode inode, next_cwd_inode;
		struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
		disk_read(&inode, sd_cwd.sfd_ino);

		for (i = 0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			for (j = 0; j < SFS_DENTRYPERBLOCK; j++) {
				if (strcmp(dir_entry[j].sfd_name, path) == 0 && dir_entry[j].sfd_ino > 0) {
					disk_read(&next_cwd_inode, dir_entry[j].sfd_ino);
					if (next_cwd_inode.sfi_type != SFS_TYPE_DIR) {
						error_message("cd", path, -2);
						return;
					}
					sd_cwd = dir_entry[j];
					found = true;
					break;
				}
			}
			if (found) break;
			error_message("cd", path, -1);
			return;
		}		
	} else {
		sd_cwd.sfd_ino = 1;		//reset at root
		sd_cwd.sfd_name[0] = '/';
		sd_cwd.sfd_name[1] = '\0';
	}
}

void sfs_ls(const char* path)
{
	int i, j, k;
	bool found = false;
	struct sfs_inode inode, base_inode, dir_entry_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
	disk_read(&inode, sd_cwd.sfd_ino);
	disk_read(&base_inode, sd_cwd.sfd_ino);

	if (path != NULL) {
		for (i = 0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			for (j = 0; j < SFS_DENTRYPERBLOCK; j++) {
				if (strcmp(dir_entry[j].sfd_name, path) == 0 && dir_entry[j].sfd_ino > 0) {
					disk_read(&base_inode, dir_entry[j].sfd_ino);
					if (base_inode.sfi_type != SFS_TYPE_DIR) {
						printf("%s\n", path);
						return;
					}
					found = true;
					break;
				}
			}
			if (found) break;
			error_message("ls", path, -1);
			return;
		}
	} 

	for (k = 0; k < SFS_NDIRECT; k++) {
		if (base_inode.sfi_direct[k] == 0) break;
		disk_read(dir_entry, base_inode.sfi_direct[k]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++) {
			if (dir_entry[j].sfd_ino > 0) {
				printf("%s", dir_entry[j].sfd_name);
				disk_read(&dir_entry_inode, dir_entry[j].sfd_ino);
				if (dir_entry_inode.sfi_type == SFS_TYPE_DIR) {
					printf("/");
				}
				printf("	");
			}
		}
	}
	printf("\n");
}



void sfs_rmdir(const char* org_path) 
{
	printf("Not Implemented\n");
}

void sfs_mv(const char* src_name, const char* dst_name) 
{
	int i, j, k, l;
	struct sfs_inode cwd;
	bool found = false;
	bool already_exist_error = false;
	disk_read(&cwd, sd_cwd.sfd_ino);
	struct sfs_inode inode, base_inode, dir_entry_inode;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];
	disk_read(&inode, sd_cwd.sfd_ino);
	disk_read(&base_inode, sd_cwd.sfd_ino);

	for (i = 0; i < SFS_NDIRECT; i++) {
		if (inode.sfi_direct[i] == 0) break;
		disk_read(dir_entry, inode.sfi_direct[i]);
		for (j = 0; j < SFS_DENTRYPERBLOCK; j++) {
			if (strcmp(dir_entry[j].sfd_name, src_name) == 0 && dir_entry[j].sfd_ino > 0) {
				disk_read(&base_inode, dir_entry[j].sfd_ino);
				found = true;
				break;
			}
		}
		if (found) break;
	}
	if (!found) {
		error_message("mv", src_name, -1);
		return;
	}

	for (k = 0; k < SFS_NDIRECT; k++) {
		if (inode.sfi_direct[k] == 0) break;
		disk_read(dir_entry, inode.sfi_direct[k]);
		for (l = 0; l < SFS_DENTRYPERBLOCK; l++) {
			if (strcmp(dir_entry[l].sfd_name, dst_name) == 0 && dir_entry[l].sfd_ino > 0) {
				error_message("mv", dst_name, -6);
				return;
			}
		}
	}

	strcpy(dir_entry[j].sfd_name, dst_name);
	disk_write(dir_entry, inode.sfi_direct[i]);

}

void sfs_rm(const char* path) 
{
	printf("Not Implemented\n");
}

void sfs_cpin(const char* local_path, const char* path) 
{
	printf("Not Implemented\n");
}

void sfs_cpout(const char* local_path, const char* path) 
{
	printf("Not Implemented\n");
}

void dump_inode(struct sfs_inode inode) {
	int i;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	printf("size %d type %d direct ", inode.sfi_size, inode.sfi_type);
	for(i=0; i < SFS_NDIRECT; i++) {
		printf(" %d ", inode.sfi_direct[i]);
	}
	printf(" indirect %d",inode.sfi_indirect);
	printf("\n");

	if (inode.sfi_type == SFS_TYPE_DIR) {
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			dump_directory(dir_entry);
		}
	}

}

void dump_directory(struct sfs_dir dir_entry[]) {
	int i;
	struct sfs_inode inode;
	for(i=0; i < SFS_DENTRYPERBLOCK;i++) {
		printf("%d %s\n",dir_entry[i].sfd_ino, dir_entry[i].sfd_name);
		disk_read(&inode,dir_entry[i].sfd_ino);
		if (inode.sfi_type == SFS_TYPE_FILE) {
			printf("\t");
			dump_inode(inode);
		}
	}
}

void sfs_dump() {
	// dump the current directory structure
	struct sfs_inode c_inode;

	disk_read(&c_inode, sd_cwd.sfd_ino);
	printf("cwd inode %d name %s\n",sd_cwd.sfd_ino,sd_cwd.sfd_name);
	dump_inode(c_inode);
	printf("\n");

}
