#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "filesys.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "merkletree.h"
#include "errno.h"
static int filesys_inited = 0;
NODE *root;
int leafIndex;
int n_nodes;
int n_blocks;
int glob_fd[100];
int sizes[100];
NODE *list;
/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash(const void *buf, int len, const void *sha1)
{
	SHA1((unsigned char *)buf, len, (unsigned char *)sha1);
}

int sameHash(char a[20], char b[20])
{
	char a_hash[21];
	memcpy(a_hash, a, 20);
	char b_hash[21];
	memcpy(b_hash, b, 20);
	a_hash[20] = '\0';
	b_hash[20] = '\0';
	// printf("%s %s\n", a_hash, b_hash);
	return strcmp(a_hash, b_hash) == 0;
}
NODE *buildTree(struct Node *nodes, int len)
{
	int h = ceil(log(len) / log(2));
	if (len == 0)
	{
		h = 0;
	}
	n_nodes = pow(2, h) + len - 1;
	// printf("%d \n", n_nodes);
	struct Node *tree = (struct Node *)malloc(sizeof(struct Node) * n_nodes);
	int l = 0;
	for (int i = 0; i < n_nodes; i++)
	{
		memset(tree[i].hash, 0, 20);
	}
	leafIndex = pow(2, h) - 1;
	for (int i = leafIndex; i < leafIndex + len; i++)
	{
		memcpy(tree[i].hash, nodes[l].hash, 20);
		l++;
	}

	for (int i = leafIndex + len - 1; i >= 0; i--)
	{
		int l_child = 2 * i + 1;
		int r_child = 2 * i + 2;
		char conchash[40];
		int hashlen = 0;
		if (l_child < n_nodes)
		{
			memcpy(conchash, tree[l_child].hash, 20);
			hashlen += 20;
		}
		if (r_child < n_nodes)
		{
			memcpy(&conchash[20], tree[r_child].hash, 20);
			hashlen += 20;
		}
		if (hashlen > 0)
		{
			get_sha1_hash(conchash, hashlen, tree[i].hash);
		}
	}
	root = tree;
	return tree;
}
/* Build an in-memory Merkle tree for the file.
 * Compare the integrity of file with respect to
 * root hash stored in secure.txt. If the file
 * doesn't exist, create an entry in secure.txt.
 * If an existing file is going to be truncated
 * update the hash in secure.txt.
 * returns -1 on failing the integrity check.
 */
int findSize(int fd)
{
	int sz = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	return sz;
}

int findIndex(int blockNo)
{
	return leafIndex + blockNo;
}
int checkBlockHash(int fd, int blockNo)
{
	int cpos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, blockNo * 64, SEEK_SET);
	char buffer[64];
	int b = read(fd, buffer, 64);
	if (b < 0)
	{
		printf("%s\n", strerror(errno));
	}
	int idx = findIndex(blockNo);
	char hash[20];
	if (b > 0)
	{
		get_sha1_hash(buffer, b, hash);
	}
	else
	{
		memset(hash, 0, 20);
	}

	lseek(fd, cpos, SEEK_SET);
	return sameHash(hash, root[idx].hash);
}
void print_hash(char hash[20])
{
	for (int i = 0; i < 20; i++)
	{
		printf("%d", hash[i]);
	}
	printf("\n");
}

int checkIfPresent(const char *pathname, char *hash)
{
	int fd = open("secure.txt", O_RDONLY);
	int k = 1;
	char filename[100];
	int i = 0;
	int pos = 0;
	while (k > 0)
	{
		char c;
		k = read(fd, &c, 1);
		// printf("%c\n", c);
		pos += k;
		if (k > 0)
		{
			if (c != ' ')
			{
				filename[i++] = c;
			}
			else
			{
				filename[i] = '\0';
				printf("%s\n", filename);
				if (strcmp(filename, pathname) == 0)
				{
					read(fd, hash, 20);
					close(fd);
					return pos;
				}
				else
				{
					lseek(fd, 21, SEEK_CUR);
					i = 0;
					memset(filename, 0, 100);
				}
			}
		}
	}
	close(fd);
	return -1;
}

NODE *buildList(int fd, int numberOfNodes)
{
	int cpos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, 0, SEEK_SET);
	NODE *nodes = (NODE *)malloc(sizeof(NODE) * numberOfNodes);
	for (int i = 0; i < numberOfNodes; i++)
	{
		char buffer[64];
		int r = read(fd, buffer, 64);
		if (r > 0)
		{
			get_sha1_hash(buffer, r, nodes[i].hash);
		}
		else
		{
			memset(nodes[i].hash, 0, 20);
		}

		// print_hash(nodes[i].hash);
		// printf("%d\n", r);
	}
	lseek(fd, cpos, SEEK_SET);
	return nodes;
}
int s_open(const char *pathname, int flags, mode_t mode)
{
	assert(filesys_inited);
	int fd = open(pathname, flags, mode);
	glob_fd[fd] = open(pathname, O_RDONLY);
	int sz = findSize(fd);
	sizes[fd] = sz;
	int numberOfNodes = ceil((double)sz / 64);
	numberOfNodes = numberOfNodes > 0 ? numberOfNodes : 1;
	NODE *nodes = (NODE *)malloc(sizeof(NODE) * numberOfNodes);
	// printf("%d %d\n", sz, numberOfNodes);
	for (int i = 0; i < numberOfNodes; i++)
	{
		char buffer[64];
		int r = read(glob_fd[fd], buffer, 64);
		if (r > 0)
		{
			get_sha1_hash(buffer, r, nodes[i].hash);
		}
		else
		{
			memset(nodes[i].hash, 0, 20);
		}

		// print_hash(nodes[i].hash);
		// printf("%d\n", r);
	}
	list = nodes;
	n_blocks = numberOfNodes;
	NODE *tree = buildTree(nodes, numberOfNodes);
	char hash[20];
	memset(hash, 0, 20);
	int is_present = checkIfPresent(pathname, hash);
	// printf("%s %d\n", pathname, is_present);
	if (is_present != -1)
	{
		if (!sameHash(hash, tree[0].hash))
		{
			return -1;
		}
	}
	else
	{
		int s_fd = open("secure.txt", O_RDWR | O_APPEND);
		int k = write(s_fd, pathname, strlen(pathname));
		// printf("%d\n", k);
		char c = ' ';
		write(s_fd, &c, 1);
		k = write(s_fd, tree[0].hash, 20);
		// printf("%d\n", errno);
		// printf("%s\n", strerror(errno));

		c = '\n';
		write(s_fd, &c, 1);
		close(s_fd);
	}
	// close(fd);
	// int i = 0;
	// char c = '1';
	// while(c!=' '){

	// }
	// printf("%s \n", pathname);
	// int f = open(pathname, flags, mode);
	return fd;
	// printf("FD is %d\n", fd);
	printf("%s\n", strerror(errno));
}

/* SEEK_END should always return the file size 
 * updated through the secure file system APIs.
 */
int s_lseek(int fd, long offset, int whence)
{
	assert(filesys_inited);
	if (whence == SEEK_END)
	{
		return lseek(fd, sizes[fd] + offset, SEEK_SET);
	}
	return lseek(fd, offset, SEEK_SET);
}

/* read the blocks that needs to be updated
 * check the integrity of the blocks
 * modify the blocks
 * update the in-memory Merkle tree and root in secure.txt
 * returns -1 on failing the integrity check.
 */

void findByHashAndUpdate(char prev_hash[20], char new_hash[20])
{
	int fd = open("secure.txt", O_RDWR);
	char hash[20];
	int k = 1;
	while (k > 0)
	{
		char c;
		k = read(fd, &c, 1);
		if (k > 0)
		{
			if (c == ' ')
			{
				int prev_pos = lseek(fd, 0, SEEK_CUR);
				read(fd, hash, 20);
				if (sameHash(hash, prev_hash))
				{
					lseek(fd, prev_pos, SEEK_SET);
					write(fd, new_hash, 20);
					break;
				}
			}
		}
	}
	close(fd);
}
void updateHash(int fd, int i)
{

	int k = i;
	// k = k/2;
	/*
		If number of nodes is greater? Resize
	 */
	int cpos = lseek(fd, 0, SEEK_CUR);
	char buffer[64];
	lseek(fd, (k)*64, SEEK_SET);
	int r = read(fd, buffer, 64);
	get_sha1_hash(buffer, r, list[k].hash);
	// while(k>=0){
	// 	int rootDone = 0;
	// 	if(k==0){
	// 		rootDone = 1;
	// 	}
	// 	int l_child = 2*k + 1;
	// 	int r_child = 2*k + 2;
	// 	char hash[40];
	// 	int hash_len = 0;
	// 	if(l_child<n_nodes){
	// 		memcpy(hash, root[l_child].hash, 20);
	// 		hash_len+=20;
	// 	}
	// 	if(r_child<n_nodes){
	// 		memcpy(&hash[20], root[r_child].hash, 20);
	// 		hash_len+=20;
	// 	}
	// 	if(hash_len>0){
	// 		get_sha1_hash(hash, hash_len, root[k].hash);
	// 	}
	// 	else{
	// 		char buffer[64];
	// 		lseek(fd, (k-leafIndex)*64, SEEK_SET);
	// 		int r = read(fd, buffer, 64);
	// 		get_sha1_hash(buffer, r, root[k].hash);
	// 	}
	// 	k = k/2;
	// 	if(rootDone){
	// 		break;
	// 	}
	// }

	lseek(fd, cpos, SEEK_SET);
}
ssize_t s_write(int fd, const void *buf, size_t count)
{
	assert(filesys_inited);
	int pos = lseek(fd, 0, SEEK_CUR);
	if (pos == -1)
	{
		printf("%s\n", strerror(errno));
	}
	int blockBegIdx = pos / 64;
	int blockEndIdx = blockBegIdx;
	if (count > (blockBegIdx + 1) * 64 - pos)
	{

		int cnt = count - ((blockBegIdx + 1) * 64 - pos);
		blockEndIdx += ceil((double)cnt / 64); // Handle remainders
	}
	for (int i = blockBegIdx; i <= ((blockEndIdx < n_blocks) ? blockEndIdx : n_blocks - 1); i++)
	{
		// printf("%d\n", i);
		if (!checkBlockHash(glob_fd[fd], i))
		{
			return -1;
		}
	}
	char prev_hash[20];
	memcpy(prev_hash, root[0].hash, 20);
	int k = write(fd, buf, count);
	if (blockEndIdx + 1 > n_blocks)
	{
		// Have to recreate tree
		NODE *newList = buildList(glob_fd[fd], blockEndIdx + 1);
		list = newList;
		n_blocks = blockEndIdx + 1;
		root = buildTree(newList, n_blocks);
	}
	for (int i = blockBegIdx; i <= blockEndIdx; i++)
	{
		updateHash(glob_fd[fd], i);
	}
	sizes[fd] = findSize(glob_fd[fd]);
	root = buildTree(list, n_blocks);
	findByHashAndUpdate(prev_hash, root[0].hash);
	return k;
}

/* check the integrity of blocks containing the 
 * requested data.
 * returns -1 on failing the integrity check.
 */
ssize_t s_read(int fd, void *buf, size_t count)
{
	assert(filesys_inited);
	int pos = lseek(fd, 0, SEEK_CUR);
	if (pos == -1)
	{
		printf("%s\n", strerror(errno));
	}
	int blockBegIdx = pos / 64;
	int blockEndIdx = blockBegIdx;
	if (count > (blockBegIdx + 1) * 64 - pos)
	{

		int cnt = count - ((blockBegIdx + 1) * 64 - pos);
		blockEndIdx += ceil((double)cnt / 64); // Handle remainders
	}
	for (int i = blockBegIdx; i <= ((blockEndIdx < n_blocks) ? blockEndIdx : n_blocks - 1); i++)
	{
		// printf("%d\n", i);
		if (!checkBlockHash(glob_fd[fd], i))
		{
			return -1;
		}
	}
	return read(fd, buf, count);
}

/* destroy the in-memory Merkle tree */
int s_close(int fd)
{
	assert(filesys_inited);
	return close(fd);
}

/* Check the integrity of all files in secure.txt
 * remove the non-existent files from secure.txt
 * returns 1, if an existing file is tampered
 * return 0 on successful initialization
 */
int filesys_init(void)
{
	filesys_inited = 1;
	int fd = open("secure.txt", O_CREAT | O_RDWR, 0755);
	char filename[100];
	char hash[20];
	int f_index = 0, h_index = 0, l = 0;
	int prev_pos = 0;
	while (1)
	{
		char c;
		int k = read(fd, &c, 1);
		if (k <= 0)
		{

			break;
		}
		if (c == ' ')
		{
			l = 1;
			continue;
		}
		if (c == '\n')
		{
			int f_fd = open(filename, O_RDONLY);
			if (f_fd == -1)
			{
				char buffer[128000];
				int r = read(fd, buffer, 128000);
				ftruncate(fd, prev_pos);
				lseek(fd, prev_pos, SEEK_SET);

				if (r != -1)
				{

					write(fd, buffer, r);
					lseek(fd, prev_pos, SEEK_SET);
				}
				prev_pos = lseek(fd, 0, SEEK_CUR);
			}
			else
			{
				f_fd = s_open(filename, O_RDONLY, S_IRUSR | S_IWUSR);
				if (f_fd == -1)
				{
					return 1;
				}
			}
			f_index = 0;
			h_index = 0;
			l = 0;
			continue;
		}
		if (!l)
		{
			filename[f_index++] = c;
		}
		else
		{
			hash[h_index++] = c;
		}
	}

	close(fd);

	/*
		Read each hash and call sopen
	 */
	return 0;
}
