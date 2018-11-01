/*
 * File: page_handle.h
 * Description: MiniDB Page interface
 * Author: 
 * E-mail:
 *
 */
#pragma once

class PageHandle
{
    private:
        int page_id;  // unique identifier of the page
        char *page_data_ptr;   // pointer to page data
    public:
        PageHandle();
        ~PageHandle();
        void getData(char *&data_prt) const;
        void getPageID(int pg_id) const;
};
