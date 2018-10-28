/*
 * File: mdb_file.h
 * Description: MiniDB File interface
 * Author:
 * E-mail:
 *
 */
#pragma once

struct FileHeader
{
    int free_page;
    int file_id;
}
class MDB_BufferMgr;

class MDB_File
{
    public:
        MDB_File();
        ~MDB_File();
        void getFirstPage(MDB_Page &mdb_page) const;
        void getNextPage(int cur_pg_id, MDB_Page &mdb_page) const;
        void getPrevPage(int cur_pg_id, MDB_Page &mdb_page) const;
        void getThisPage(int pg_id, MDB_Page &mdb_page) const;
        void getLastPage(MDB_Page &mdb_page) const;
        void allocatePage(MDB_Page &mdb_page);
        void disposePage(int pg_id);
        void markDirty(int pg_id);
        void unpinPage(int pg_id);
        // Flush pages into disk from buffer pool
        void flushPages() const;
        // Write a page or pages to disk, but do not remove from buffer pool
        void writePages(int pg_id = -1);    // '-1' means all pages
    private:
        MDB_BufferMgr *buffer_mgr_ptr;
        FileHeader hdr;
        int file_open;
        int if_dirty;
        int sysfd;
};
