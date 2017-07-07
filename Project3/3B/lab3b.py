#!/usr/bin/python

import sys
import string
import locale
import csv

code = 0 #return code

superblock = {}
first_non_reserved_block = 0

#block audit vars
names = ["", "INDIRECT ", "DOUBLE INDIRECT ", "TRIPPLE INDIRECT "]
blocks = []

#inode audit vars
allocated_inodes = []
unallocated_inodes = []
free_inodes = []

#directory audit vars
inode_links = {}
inode_p = {}

def read_csv():
    global superblock
    global blocks
    global first_non_reserved_block

    with open(sys.argv[1], 'r') as file:
        csv_super_group_bfree = csv.reader(file, delimiter=',')
        for line in csv_super_group_bfree:
           if (line[0] == "SUPERBLOCK"):
               superblock['nBlocks'] = int(line[1])
               superblock['nInodes'] = int(line[2])
               superblock['block_size'] = int(line[3])
               superblock['inode_size'] = int(line[4])
               superblock['first_inode'] = int(line[7])
               blocks = [[0 for i in range(3)] for j in range(int(line[1]))]
           elif (line[0] == "GROUP"):#get first non reserved block number
                first_non_reserved_block = int(line[8])
                #please check first_non_reserved_block. I am not really sure about this.
                #I saw my friend did and he add something to it
           elif (line[0] == "BFREE"):#update free blocks and mard as -1 in "groups" list
                blocks[int(line[1])][0] = -1

    first_non_reserved_block = first_non_reserved_block + int(superblock['inode_size'] * superblock['nInodes'] / superblock['block_size'])
#end read_csv

def block_audits():
    with open(sys.argv[1], 'r') as file:
        csv_inode = csv.reader(file, delimiter=',')
        for line in csv_inode:
            if (line[0] == "INODE"):
                if (int(line[3]) != 0): #if valid
                    inodeNum = int(line[1])
                    for i in range(15):
                        if (i < 12):
                            level = 0
                            offset = i
                        elif (i == 12):
                            level = 1
                            offset = i
                        elif (i == 13):
                            level = 2
                            offset = 268
                        elif (i == 14):
                            level = 3
                            offset = 65804
                        blocknum = int(line[i + 12])
                        #invalid
                        if (blocknum < 0 or blocknum >= superblock['nBlocks']):
                            print "INVALID %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                            code = 2
                        elif (blocknum > 0):
                            #reserved
                            if (blocknum < first_non_reserved_block):
                                print "RESERVED %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                                code = 2
                            #duplicates
                            if (blocks[blocknum][0] == -1):
                                blocks[blocknum][0] = inodeNum
                                blocks[blocknum][1] = level
                                blocks[blocknum][2] = offset
                                print "ALLOCATED BLOCK %d ON FREELIST"%blocknum
                                code = 2
                            elif (blocks[blocknum][0] == 0):
                                blocks[blocknum][0] = inodeNum
                                blocks[blocknum][1] = level
                                blocks[blocknum][2] = offset
                            elif (blocks[blocknum][0] > 0):
                                print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                                print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[blocks[blocknum][1]], blocknum, blocks[blocknum][0], blocks[blocknum][2])
                                blocks[blocknum][0] = -3 #flag
                                code = 2
                            elif (blocks[blocknum][0] == -3):
                                print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                                code = 2
        
    with open(sys.argv[1], 'r') as file:
        csv_indirect = csv.reader(file, delimiter=',')
        for line in csv_indirect:
            if (line[0] == "INDIRECT"):
                inodeNum = int(line[1])
                level = int(line[2])
                offset = int(line[3])
                blocknum = int(line[5])
                #invalid
                if (blocknum  < 0 or blocknum >= superblock['nBlocks']):
                    print "INVALID %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                    code = 2
                elif (blocknum > 0):
                    #reserved
                    if (blocknum < first_non_reserved_block):
                        print "RESERVED %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                        code = 2
                    #duplicates
                    if (blocks[blocknum][0] == -1): #free block
                        blocks[blocknum][0] = inodeNum
                        blocks[blocknum][1] = level
                        blocks[blocknum][2] = offset
                        print "ALLOCATED BLOCK %d ON FREELIST"%blocknum
                        code = 2
                    elif (blocks[blocknum][0] == 0): #update
                        blocks[blocknum][0] = inodeNum
                        blocks[blocknum][1] = level
                        blocks[blocknum][2] = offset
                    elif (blocks[blocknum][0] > 0): #if duplicate at the first time, print both
                        print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                        print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[blocks[blocknum][1]], blocknum, blocks[blocknum][0], blocks[blocknum][2])
                        blocks[blocknum][0] = -3 #set the flag
                        code = 2
                    elif (blocks[blocknum][0] == -3): #if duplicate more than one time, print new one
                        print "DUPLICATE %sBLOCK %d IN INODE %d AT OFFSET %d"%(names[level], blocknum, inodeNum, offset)
                        code = 2
    
    for i in range(first_non_reserved_block, superblock['nBlocks']):
        if (blocks[i][0] == 0):
            print "UNREFERENCED BLOCK %d"%i
            code = 2
#end block_audits()

def inode_audits():
    global allocated_inodes
    global unallocated_inodes
    global free_inodes
    global superblock
    #fill up inode lists
    with open(sys.argv[1], 'r') as file:
        csvinode = csv.reader(file, delimiter=',')
        for line in csvinode:
            if (line[0] == "INODE"):
                if (int(line[3]) != 0):
                    allocated_inodes.append(int(line[1]))
            elif (line[0] == "IFREE"):
                free_inodes.append(int(line[1]))

    #iterate over all available inodes and put in unallocated if not allocated
    for x in range(superblock['first_inode'],superblock['nInodes']):
        if (x not in allocated_inodes):
            unallocated_inodes.append(x)
        
    #check if allocated inodes are in the free list        
    for inode in allocated_inodes:
        if (inode in free_inodes):
            print "ALLOCATED INODE %s ON FREELIST"%inode
            code = 2

    #check if unallocated inodes are no on the free list
    for inode in unallocated_inodes:
        if (inode not in free_inodes):
            print "UNALLOCATED INODE %s NOT ON FREELIST"%inode
            code = 2
#end inode_audits

def directory_audits():
    global superblock
    global unallocated_inodes
    global inode_links
    global inode_p

    #count how many times each inode is linked (don't even record if no links)
    with open(sys.argv[1], 'r') as file:
        csvdirent = csv.reader(file, delimiter=',')
        for line in csvdirent:
            if (line[0] == "DIRENT"):
                if (inode_links.has_key(int(line[3])) == False):
                    inode_links[int(line[3])] = 1
                    inode_p[int(line[3])] = int(line[1])
                else:
                    inode_links[int(line[3])] += 1
                #invalid/unallocated
                if (int(line[3]) > superblock['nInodes']):
                    print "DIRECTORY INODE %d NAME %s INVALID INODE %d"%(int(line[1]),line[6], int(line[3]))
                    code = 2      
                elif (int(line[3]) in unallocated_inodes):
                    print "DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d"%(int(line[1]),line[6], int(line[3]))
                    code = 2

    #find all link inconsistencies
    with open(sys.argv[1], 'r') as file:
        csvinode = csv.reader(file, delimiter=',')
        for line in csvinode:
            if (line[0] == "INODE"):
                inodeNum = int(line[1])
                linkCount = int(line[6])
                if ( (inode_links.has_key(inodeNum) == False) and (linkCount != 0) ):
                    print "INODE %d HAS 0 LINKS BUT LINKCOUNT IS %d"%(inodeNum,linkCount)
                    code = 2
                elif (inode_links.has_key(inodeNum)):
                    if (inode_links[inodeNum] != linkCount):
                        if (inode_links[inodeNum] == 1):
                            print "INODE %d HAS 1 LINK BUT LINKCOUNT IS %d"%(inodeNum,linkCount)
                            code = 2
                        else:
                            print "INODE %d HAS %d LINKS BUT LINKCOUNT IS %d"%(inodeNum,inode_links[inodeNum],linkCount)
                            code = 2                                    

    #for each inconsistency with '..' and '.'                
    with open(sys.argv[1], 'r') as file:
        csvdirent1 = csv.reader(file, delimiter=',')
        for line in csvdirent1:
            if (line[0] == "DIRENT"): #found a dirent entry
                if (line[6] == "'.'"):
                    if (line[1] != line[3]):
                        print "DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d"%(int(line[1]), int(line[3]), int(line[1]))
                        code = 2
                elif (line[6] == "'..'"):
                    if (int(line[3]) != inode_p[int(line[1])]):
                        print "DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d"%(int(line[1]), int(line[3]), inode_p[int(line[3])])
                        code = 2
#end directory_audits


def main():
    #check if there exist bad parameters. If so, exit with code 1
    if (len(sys.argv) != 2):
        print >> sys.stderr, 'Invalid arguments'
        sys.exit(1)
    try:
        with open(sys.argv[1], 'r') as file:
            check = csv.reader(file, delimiter=',')
    except:
        print >> sys.stderr, 'Unable to open required files'
        sys.exit(1)

    #otherwise, execute
    read_csv()
    block_audits()
    inode_audits()
    directory_audits()
    sys.exit(code)

#end main

if __name__ == "__main__":
    main()
