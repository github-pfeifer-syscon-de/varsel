/*
 * Copyright (C) 2025 RPf 
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

// this is a check the output test

#include <iostream>
#include <iomanip>
#include <glibmm.h>
#include <giomm.h>

static bool
testList()
{
    Glib::RefPtr<Gio::FileEnumerator> enumerat;
    bool ret = true;
    try {
        auto home = Gio::File::create_for_path(Glib::get_current_dir());
        auto temp = home->get_child("temp");
        if (temp->query_exists()) {
            std::cout << "The directory " << temp->get_path() << " already exists no test taken!" << std::endl;
            return true;
        }
        temp->make_directory();
        auto test = temp->get_child("test.txt");
        auto cancellable = Gio::Cancellable::create();
        if (!test->query_exists()) {
            auto outs = test->append_to(Gio::FileCreateFlags::FILE_CREATE_REPLACE_DESTINATION);
            outs->write("Helord\n");
            outs->close();
        }
        auto link = temp->get_child("link");
        if (!link->query_exists()) {
            link->make_symbolic_link("../FileTest.cpp");
        }
        enumerat = temp->enumerate_children(
              cancellable
            , "*"
            , Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS);
        while (true) {
            auto fileInfo = enumerat->next_file();
            if (!fileInfo) {
                break;
            }
            auto child = temp->get_child(fileInfo->get_name());
            auto childInfo = child->query_info("*", Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE);
            // this varies depending on the existence of the target (without all give type = link)
            //   and for just created files ???
            std::cout << "name "  << std::setw(16) << fileInfo->get_name()
                      << " info type " << fileInfo->get_file_type()
                      << " child nof type " << child->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NOFOLLOW_SYMLINKS)
                      << " child non type " << child->query_file_type(Gio::FileQueryInfoFlags::FILE_QUERY_INFO_NONE)
                      << " child info type  " << childInfo->get_file_type()
                      << " target " << (childInfo->has_attribute(G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET) ? childInfo->get_symlink_target() : std::string(""))
                      << std::endl;
            for (std::string attr : childInfo->list_attributes()) {
                Glib::ustring typeName;
                auto type = childInfo->get_attribute_type(attr);
                Glib::ustring value = childInfo->get_attribute_as_string(attr);
                switch (type) {
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_BOOLEAN:
                        typeName = "Boolean";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_BYTE_STRING:
                        typeName = "Byte string";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_INT32:
                        typeName = "Int32";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_INT64:
                        typeName = "Int64";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_INVALID:
                        typeName = "Invalid";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_OBJECT:
                        typeName = "Object";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_STRING:
                        typeName = "String";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_STRINGV:
                        typeName = "String Array";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_UINT32:
                        typeName = "Uint32";
                        break;
                    case Gio::FileAttributeType::FILE_ATTRIBUTE_TYPE_UINT64:
                        typeName = "Uint64";
                        break;
                }
                std::cout << "   " << attr << " = " << value << " (" << typeName << ")" << std::endl;
            }

            child->remove();
        }
        temp->remove();
    }
    catch (const Gio::Error& err) {
        std::cout << "Error " << err.what() << std::endl;
        ret = false;
    }
    if (enumerat
     && !enumerat->is_closed()) {
        enumerat->close();
    }
    return ret;
}

static bool
buildUstring()
{
    // provoke some invalid utf-8 encoding
    std::string str{"xx01234"};
    str[0] = static_cast<char>(0b11000000);
    str[1] = static_cast<char>(0b11000000);

    //std::cout << "str " << str << " len " << str.length() << std::endl;

    Glib::ustring ustring{Glib::strescape(str)};    // when creating from string directly this blows up...

    std::cout << "ustr " << ustring << " len " << ustring.length() << std::endl;

    return true;
}

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    Gio::init();    //  make Gio::FileInputStream work!


    if (!testList()) {
        return 1;
    }
    if (!buildUstring()) {
        return 1;
    }


    return 0;
}
