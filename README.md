# SecureFS
My implementation of a Secure File System based on Merkle Trees (IIITD OS Assignment 4)

The assignment: https://www.usebackpack.com/resources/22414/download?1554605186

The "secure" comes from the use of Merkle Trees, which we are using to calculate the hashes of file blocks (size - 64 bytes).
Using the SHA Hashing function, we can compute the hash of any block, which we're using to verify the cryptographic integrity of the filesystem.

It consists of the following functions:
1. Filesys_init: It creates a secure.txt file if it is not present. If present, it goes through the secure.txt file and verifies the integrity of each file present 
in it. If the file has been deleted, the entry is removed. If its integrity has been compromised, Filesys_init returns 0.

2. s_open: It creates the in memory Merkle Tree of the given file and compares the root hash with the entry in secure.txt. If there's no entry, it creates one.

3. s_write: It checks which blocks are being written to, and then computes the hash of these blocks with the in memory tree. If there's been any compromise, it returns -1. 
It then writes the blocks and updates those blocks in the tree.

4. s_read: It just checks that the blocks to be read haven't been compromised and then reads those blocks if that is the case

The existing template (and testcases) have been provided by the course instructor, I've just filled in the functions in filesys.c.

