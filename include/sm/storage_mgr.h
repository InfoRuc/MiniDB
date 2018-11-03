/*
 * File: storage_mgr.h
 * Description: File handle interface
 * Author: Liu Chaoyang
 * E-mail: chaoyanglius@gmail.com
 *
 */
#pragma once

struct FileHeader
{
    int first_free;
    int pages_num;
};

class BufferMgr;

//
// Class: StorageMgr
// Description: manage disk and buffer. Using a big file to storage all record.
// Author: Liu Chaoyang
// E-mail: chaoyanglius@gmail.com
//
class StorageMgr
{
    private:
        FileHeader hdr;
        bool file_open;
        bool hdr_changed;
        int sys_fd;
        BufferMgr *buffer_mgr;
    public:
        StorageMgr();
        // copy constructor
        StorageMgr(const StorageMgr &storage_mgr);
        // assignment of StorageMgr by overload '='
        StorageMgr& operator=(const StorageMgr &storage_mgr);
        ~StorageMgr();
        bool getFirstPage(PageHandle &page_handle);
        bool getNextPage(int cur_pg_id, PageHandle &page_handle);
        bool getPrevPage(int cur_pg_id, PageHandle &page_handle);
        // get a page by its id and get its handle
        bool getThisPage(int pg_id, PageHandle &page_handle);
        bool getLastPage(PageHandle &page_handle);
        bool allocatePage(PageHandle &page_handle);
        bool disposePage(int pg_id);
        bool markDirty(int pg_id);
        bool unpinPage(int pg_id);
        // Flush pages into disk from buffer pool
        bool flushPages();
        // Write a page or pages to disk, but do not remove from buffer pool
        bool forcePages(int pg_id = -1);    // '-1' means all pages
};
