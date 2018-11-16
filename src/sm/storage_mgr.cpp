/*
 * File: storage_mgr.cpp
 * Description: File handle implemention
 * Author:
 * E-mail:
 *
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include "common.h"
#include "sm/page_handle.h"
#include "sm/storage_mgr.h"
#include "sm/buffer_mgr.h"
#include "sm/page_hashtable.h"

using std::cout;
using std::endl;

StorageMgr::StorageMgr()
{
    buffer_mgr = new BufferMgr(PAGE_BUFFER_SIZE);
    file_open = false;
}

StorageMgr::~StorageMgr()
{
    delete buffer_mgr;
}

StorageMgr::StorageMgr(const StorageMgr &storage_mgr)
{
    buffer_mgr = storage_mgr.buffer_mgr;
    hdr = storage_mgr.hdr;
    file_open = storage_mgr.file_open;
    hdr_changed = storage_mgr.hdr_changed;
    sys_fd = storage_mgr.sys_fd;
}

StorageMgr& StorageMgr::operator=(const StorageMgr &storage_mgr)
{
    buffer_mgr = storage_mgr.buffer_mgr;
    hdr = storage_mgr.hdr;
    file_open = storage_mgr.file_open;
    hdr_changed = storage_mgr.hdr_changed;
    sys_fd = storage_mgr.sys_fd;

    return *this;
}

bool StorageMgr::initSM(const char *file_name)
{
    int sys_fd = -1;
    int offset = 0;
    int bytes_num;

    // init file header
    hdr.first_free = -1;
    hdr.pages_num = 0;
    sys_fd = open(file_name, O_RDWR, 0600);
    if (sys_fd < 0)
    {
#ifdef DEBUG
        cout << "[StorageMgr Debug]: " << "DB file does not exist." << endl;
        cout << "[StorageMgr Debug]: " << "Creating DB file..." << endl;
#endif
        sys_fd = open(file_name, O_CREAT | O_RDWR, 0600);
        if (sys_fd < 0)
        {
            cout << "Error: open file failed!" << endl;
            return false;
        }
        hdr.first_free = -1;
        hdr.pages_num = 0;
        memset(hdr.ds_pt, 0, DATA_SEGMENT_SIZE / 8);
        memset(hdr.ix_pt, 0, INDEX_SEGMENT_SIZE / 8);
        memset(hdr.ls_pt, 0, LONG_SEGMENT_SIZE / 8);
        memset(hdr.rb_pt, 0, ROLLBACK_SEGMENT_SIZE / 8);
        memset(hdr.ts_pt, 0, TEMP_SEGMENT_SIZE / 8);
        int file_header_size = FILE_HEADER_SIZE;
#ifdef DEBUG
        cout << "[StorageMgr Debug]: " << "Writing file header..." << endl;
#endif
        bytes_num = write(sys_fd, &hdr.first_free, sizeof(int));
        if (bytes_num != sizeof(int))
        {
            cout << "Error: write file header failed" << endl;
            close(sys_fd);
            return false;
        }
        bytes_num = write(sys_fd, &hdr.pages_num, sizeof(int));

        if (bytes_num != sizeof(int))
        {
            cout << "Error: write file header failed" << endl;
            close(sys_fd);
            return false;
        }
        offset = 4096;
        bytes_num = write(sys_fd, hdr.ds_pt, DATA_SEGMENT_SIZE / 8);
        if (bytes_num < DATA_SEGMENT_SIZE / 8)
        {
            cout << "[StorageMgr Error]: Write file header failed." << endl;
            return false;
        }
        offset += DATA_SEGMENT_SIZE / 8;
        if (lseek(sys_fd, offset, SEEK_SET) < 0)
        {
            cout << "[StorageMgr Error]: Seek page failed." << endl;
            return false;
        }
        bytes_num = write(sys_fd, hdr.ix_pt, INDEX_SEGMENT_SIZE / 8);
        if (bytes_num < INDEX_SEGMENT_SIZE / 8)
        {
            cout << "[StorageMgr Error]: Write file header failed." << endl;
            return false;
        }
        offset += INDEX_SEGMENT_SIZE / 8;
        if (lseek(sys_fd, offset, SEEK_SET) < 0)
        {
            cout << "[StorageMgr Error]: Write file header failed." << endl;
            return false;
        }
        bytes_num = write(sys_fd, hdr.ls_pt, LONG_SEGMENT_SIZE / 8);
        if (bytes_num < LONG_SEGMENT_SIZE / 8)
        {
            cout << "[StorageMgr Error]: Write file header failed." << endl;
            return false;
        }

        offset += LONG_SEGMENT_SIZE / 8;
        if (lseek(sys_fd, offset, SEEK_SET) < 0)
        {
            cout << "[StorageMgr Error]: Seek page failed." << endl;
            return false;
        }
        bytes_num = write(sys_fd, hdr.rb_pt, ROLLBACK_SEGMENT_SIZE / 8);
        if (bytes_num < ROLLBACK_SEGMENT_SIZE / 8)
        {
            cout << "[StorageMgr Error]: Write file header failed." << endl;
            return false;
        }
        offset += ROLLBACK_SEGMENT_SIZE / 8;
        if (lseek(sys_fd, offset, SEEK_SET) < 0)
        {
            cout << "[StorageMgr Error]: Seek page failed." << endl;
            return false;
        }
        bytes_num = write(sys_fd, hdr.ts_pt, TEMP_SEGMENT_SIZE / 8);
        if (bytes_num < TEMP_SEGMENT_SIZE / 8)
        {
            cout << "[StorageMgr Error]: Write file header failed." << endl;
            return false;
        }
    }

    bytes_num = read(sys_fd, &hdr.first_free, sizeof(int));
    if (bytes_num < sizeof(int))
    {
        cout << "[StorageMgr Error]: Read file header(first_free) failed." << endl;
        return false;
    }
    bytes_num = read(sys_fd, &hdr.pages_num, sizeof(int));
    if (bytes_num < sizeof(int))
    {
        cout << "[StorageMgr Error]: Read file header(pages_num) failed." << endl;
        return false;
    }
    offset = 4096;
    if (lseek(sys_fd, offset, SEEK_SET) < 0)
    {
        cout << "[StorageMgr Error]: Seek page failed." << endl;
        return false;
    }
    bytes_num = read(sys_fd, hdr.ds_pt, DATA_SEGMENT_SIZE / 8);
    if (bytes_num < DATA_SEGMENT_SIZE / 8)
    {
        cout << "[StorageMgr Error]: Read file header failed." << endl;
        return false;
    }
    offset += DATA_SEGMENT_SIZE / 8;
    if (lseek(sys_fd, offset, SEEK_SET) < 0)
    {
        cout << "[StorageMgr Error]: Seek page failed." << endl;
        return false;
    }
    bytes_num = read(sys_fd, hdr.ix_pt, INDEX_SEGMENT_SIZE / 8);
    if (bytes_num < INDEX_SEGMENT_SIZE / 8)
    {
        cout << "[StorageMgr Error]: Read file header failed." << endl;
        return false;
    }
    offset += INDEX_SEGMENT_SIZE / 8;
    if (lseek(sys_fd, offset, SEEK_SET) < 0)
    {
        cout << "[StorageMgr Error]: Seek page failed." << endl;
        return false;
    }
    bytes_num = read(sys_fd, hdr.ls_pt, LONG_SEGMENT_SIZE / 8);
    if (bytes_num < LONG_SEGMENT_SIZE / 8)
    {
        cout << "[StorageMgr Error]: Read file header failed." << endl;
        return false;
    }
    offset += LONG_SEGMENT_SIZE / 8;
    if (lseek(sys_fd, offset, SEEK_SET) < 0)
    {
        cout << "[StorageMgr Error]: Seek page failed." << endl;
        return false;
    }
    bytes_num = read(sys_fd, hdr.rb_pt, ROLLBACK_SEGMENT_SIZE / 8);
    if (bytes_num < ROLLBACK_SEGMENT_SIZE / 8)
    {
        cout << "[StorageMgr Error]: Read file header failed." << endl;
        return false;
    }
    offset += ROLLBACK_SEGMENT_SIZE / 8;
    if (lseek(sys_fd, offset, SEEK_SET) < 0)
    {
        cout << "[StorageMgr Error]: Seek page failed." << endl;
        return false;
    }
    bytes_num = read(sys_fd, hdr.ts_pt, TEMP_SEGMENT_SIZE / 8);
    if (bytes_num < TEMP_SEGMENT_SIZE / 8)
    {
        cout << "[StorageMgr Error]: Read file header failed." << endl;
        return false;
    }
    //memset(file_header, 0, file_header_size);
    //FileHeader* hdr_buf = (FileHeader*)file_header;
    //hdr_buf->first_free = hdr.first_free;
    //hdr_buf->pages_num = hdr.pages_num;
    
    hdr_changed = false;
    file_open = true;

    return true;
}

bool StorageMgr::getThisPage(int page_id, PageHandle &page_handle) 
{
    char *page_buf_ptr;
    if (!file_open)
    {
        return false;
    }

    if (page_id > hdr.pages_num || page_id < 0)
    {
        return false;
    }

    // get page by page id
    if (!buffer_mgr->getPage(sys_fd, page_id, page_buf_ptr));
    {
        return false;
    }

    // if page is valid
    if (((PageHdr*)page_buf_ptr)[0] == PAGE_USED)
    {
        page_handle.page_id = page_id;
        page_handle.page_data_ptr = page_buf_ptr + sizeof(PageHdr);
        return true;
    }
    
    // if page is invalid
    if (!unpinPage(page_id))
    {
        return false;
    }

    return false;
}

bool StorageMgr::getFirstPage(PageHandle &page_handle) 
{
    return getNextPage(-1, page_handle);
}

bool StorageMgr::getLastPage(PageHandle &page_handle) 
{
    return getPrevPage(hdr.pages_num, page_handle);
}

bool StorageMgr::getNextPage(int cur_pg_id, PageHandle &page_handle) 
{
    // file must be open
    if (!file_open)    
    {
        cout << "Error: File not open!" << endl;
        return false;
    }

    if (cur_pg_id != -1 && (cur_pg_id > hdr.pages_num || cur_pg_id < 0))
    {
        cout << "Error: Page ID is invalid!" << endl;
        return false;
    }

    // scan file until a valid used page is found
    for (cur_pg_id++; cur_pg_id < hdr.pages_num; cur_pg_id++)
    {
        if (getThisPage(cur_pg_id, page_handle))
        {
#ifdef DEBUG
            cout << "DEBUG: Valid page found." << endl;
#endif
            return true;
        }
        else
        {
            cout << "Error: Invalid page!" << endl;
            return false;
        }
    }

    cout << "Error: No valid page found!" << endl;
    return false;
}

bool StorageMgr::getPrevPage(int cur_pg_id, PageHandle &page_handle) 
{
    // file must be open
    if (!file_open)    
    {
        cout << "Error: File not open!" << endl;
        return false;
    }

    // check if invalid
    if (cur_pg_id != hdr.pages_num && (cur_pg_id > hdr.pages_num || cur_pg_id < 0))
    {
        cout << "Error: Page ID is invalid!" << endl;
        return false;
    }

    // scan file until a valid used page is found
    for (cur_pg_id--; cur_pg_id < hdr.pages_num; cur_pg_id--)
    {
        if (getThisPage(cur_pg_id, page_handle))
            return true;
        else
        {
            cout << "Error: Invalid page!" << endl;
            return false;
        }
    }

    cout << "Error: No valid page found!" << endl;
    return false;
}

bool StorageMgr::disposePage(int page_id)
{
    char *page_buf_ptr;

    // check if invalid
    if (page_id > hdr.pages_num || page_id < 0)
    {
        return false;
    }

    // get the page to be disposed
    if (!buffer_mgr->getPage(sys_fd, page_id, page_buf_ptr))
    {
        return false;
    }

    // if page is valid
    if (((PageHdr*)page_buf_ptr)[0] != PAGE_USED)
    {
        if (!unpinPage(page_id))
        {
            return false;
        }
        cout << "Error: Page already free!" << endl;
        return false;
    }
    
    // put the page into free list
    ((PageHdr*)page_buf_ptr)[0] = hdr.first_free;
    hdr.first_free = page_id;
    hdr_changed = true;
    
    // page has been dirty
    if (!markDirty(page_id))
    {
        return false;
    }

    // unpin the page
    if (!unpinPage(page_id))
    {
        return false;
    }

    return true;
}

bool StorageMgr::allocatePage(PageHandle &page_handle)
{
    int page_id;    // new page id
    char *page_buf_ptr;

    if (!file_open)
    {
        return false;
    }

    // if free list is not empty
    if (hdr.first_free != -1)
    {
        page_id = hdr.first_free;

        // make the first free page into buffer
        if (!buffer_mgr->getPage(sys_fd, page_id, page_buf_ptr))
        {
            return false;
        }
        hdr.first_free = ((PageHdr*)page_buf_ptr)[0];
    }
    else
    {
        page_id = hdr.pages_num;

        // allocate new page
        if (!buffer_mgr->allocatePage(sys_fd, page_id, page_buf_ptr))
        {
            return false;
        }
        hdr.pages_num++;
    }

    hdr_changed = true;

    // mark the page used
    ((PageHdr*)page_buf_ptr)[0] = PAGE_USED;

    // zero out the page data
    memset(page_buf_ptr + sizeof(PageHdr), 0, PAGE_DATA_SIZE);

    // mark dirty
    if (!markDirty(page_id))
        return false;

    page_handle.page_id = page_id;
    page_handle.page_data_ptr = page_buf_ptr + sizeof(PageHdr);

    return true;
}

bool StorageMgr::markDirty(int page_id)
{
    // file must be open
    if (!file_open)
    {
        return false;
    }

    // check if invalid
    if (page_id > hdr.pages_num || page_id < 0)
    {
        return false;
    }
    
    // let buffer manager to mark page dirty
    return buffer_mgr->markDirty(sys_fd, page_id);
}

bool StorageMgr::unpinPage(int page_id)
{
    // file must be open
    if (!file_open)
    {
        return false;
    }

    // check if invalid
    if (page_id > hdr.pages_num || page_id < 0)
    {
        return false;
    }
    
    // let buffer manager to mark page dirty
    return buffer_mgr->unpinPage(sys_fd, page_id);
}

bool StorageMgr::flushPages() 
{
    // file must be open
    if (!file_open)
    {
        return false;
    }

    if (hdr_changed)
    {
        if (lseek(sys_fd, 0, SEEK_SET) < 0)
        {
            return false;
        }

        int bytes_num = write(sys_fd, (char*)&hdr, sizeof(FileHeader));
        if (bytes_num < 0 || bytes_num != sizeof(FileHeader))
        {
            return false;
        }
        
        hdr_changed = false;
    }

    return buffer_mgr->flushPages(sys_fd);
}

bool StorageMgr::forcePage(int page_id)    
{
    // file must be open
    if (!file_open)
    {
        return false;
    }

    if (hdr_changed)
    {
        if (lseek(sys_fd, 0, SEEK_SET) < 0)
        {
            return false;
        }

        int bytes_num = write(sys_fd, (char*)&hdr, sizeof(FileHeader));
        if (bytes_num < 0 || bytes_num != sizeof(FileHeader))
        {
            return false;
        }
        
        hdr_changed = false;
    }

    return buffer_mgr->forcePage(sys_fd, page_id);
}
