/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
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

#include <clocale>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <git2.h>
#include <giomm.h>

#include "GitRepository.hpp"
#include "Git_test.hpp"

// this is a "manual" test

Git_test::Git_test()
{

	git_libgit2_init();
}


bool
Git_test::reposCpp()
{
    std::cout << "Git_test::reposCpp ----------" << std::endl;
    try {
        psc::git::Repository repo("..");
        auto commit = repo.getSingelCommit("HEAD^{commit}");
        std::cout << commit->getMessage()  << std::endl;
        auto sig = commit->getSignature();
        std::cout << "    " << sig->getAuthor() << " <" << sig->getEmail() << ">" << std::endl;
        std::cout << "    " << sig->getWhen().format_iso8601() << std::endl;

        std::cout << "On branch " << repo.getBranch() << std::endl;
        std::cout << "Status\tIndex\t\t\tWorkdir" << std::endl;
        auto status = repo.getStatus();
        for (auto iter = status.begin(); iter != status.end(); ++iter) {
            auto stat = *iter;
            std::cout << stat.to_string() << std::endl;
        }
        auto iters = repo.getIter("refs/heads/*");
        std::cout << "Iterator refs/heads/*" << std::endl;
        for (auto iter : iters) {
            std::cout << iter << std::endl;
        }
        auto remote = repo.getRemote("origin");
        std::cout << "Remote name: " << remote.getName() << "\n"
                  << "   url: " << remote.getUrl() << "\n"
                  << "   pushUrl: " << remote.getPushUrl() << std::endl;
    }
    catch (const psc::git::GitException& exc) {
        std::cout << "Error " << exc.what() << std::endl;
        return false;
    }
    auto home = Glib::get_home_dir();
    std::cout << "home " << home << std::endl;
    return true;
}

bool
Git_test::test_something()
{



    return true;
}

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    Git_test git_test;
    if (!git_test.reposCpp()) {
        return 2;
    }
    if (!git_test.test_something()) {
        return 3;
    }


    return 0;
}

