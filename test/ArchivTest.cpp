/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// this is a "manual" test

#include <iostream>
#include <glibmm.h>
#include <giomm.h>

#include "ArchivTest.hpp"

class TestArchivProvider
: public ArchivFileProvider
{
public:
    TestArchivProvider(const Glib::RefPtr<Gio::File>& dir)
    : ArchivFileProvider(dir, true)
    {
    }
    virtual ~TestArchivProvider() = default;
    virtual bool isFilterEntry(const Glib::RefPtr<Gio::File>& file) override
    {
        if (file->get_basename().ends_with(".cpp")
         || file->get_basename().ends_with(".hpp")) {
            ++m_createEntries;
            std::cout << "TestArchivProvider::isFilterEntry using " << file->get_path() << std::endl;
            return true;
        }
        return false;
    }

    int m_createEntries{0};

};

ArchivTest::ArchivTest()
{
}

bool
ArchivTest::readTest()
{
//    auto home = Glib::get_home_dir();
//    auto homeDir = Gio::File::create_for_path(home);
//    auto file = homeDir->get_child("");   // if you want to check readability for a format put it here
//    Archiv archive(file);
//    try {
//        archive.read(this);
//        for (auto fmt : archive.getReadFormats()) {
//            std::cout << "Fmt " << fmt << std::endl;
//        }
//    }
//    catch (const ArchivException& exc) {
//        std::cout << exc.what() << std::endl;
//        return false;
//    }

    return true;
}

bool
ArchivTest::readWrite()
{
    m_entries = 0;
    m_final = 0;
    auto file = Gio::File::create_for_path("test.tgz");
    Archiv archive_write(file);
    archive_write.addWriteFormat(ARCHIVE_COMPRESSION_GZIP);
    archive_write.addWriteFormat(ARCHIVE_FORMAT_TAR_PAX_RESTRICTED);
    //archive_write.addFormat(ARCHIVE_FORMAT_ZIP);
    auto dir = Gio::File::create_for_path("..");
    TestArchivProvider testArchivProvider{dir};
    archive_write.write(&testArchivProvider);

    Archiv archive(file);
    try {
        archive.read(this);
        for (auto fmt : archive.getReadFormats()) {
            std::cout << "Fmt " << fmt << std::endl;
        }
    }
    catch (const ArchivException& exc) {
        std::cout << exc.what() << std::endl;
    }
    file->remove();         // cleanup
    if (testArchivProvider.m_createEntries != m_entries
     || m_entries != m_final) {
        std::cout << "Created " << testArchivProvider.m_createEntries
                  << " or the internal count " << m_entries
                  << " and summary " << m_final
                  << " do not match!" << std::endl;
        return false;
    }
    return true;
}


void
ArchivTest::archivUpdate(const std::shared_ptr<ArchivEntry>& entry)
{
    ++m_entries;
    std::cout << "entry " << entry->getPath() << std::endl;
}

void
ArchivTest::archivDone(ArchivSummary archivSummary, const Glib::ustring& errMsg)
{
    m_final = archivSummary.getEntries() ;
    std::cout << "summary " << archivSummary.getEntries() << std::endl;
    if (!errMsg.empty()) {
        std::cout << "Error " << errMsg << "!!!" << std::endl;
    }
}


int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    Gio::init();    //  make Gio::FileInputStream work!

    ArchivTest archivTest;
    if (!archivTest.readTest()) {
        return 1;
    }
    if (!archivTest.readWrite()) {
        return 2;
    }

    return 0;
}
