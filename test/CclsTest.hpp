/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

#include <glibmm.h>

// This test is rather about checking the output
//   as practical results may vary...

class RpcLaunch;
class CclsTest;

class CclsTestInit
: public CclsInit
{
public:
    CclsTestInit(RpcLaunch* rpcLaunch, CclsTest* cclsTest, const Glib::RefPtr<Gio::File>& file);
    explicit CclsTestInit(const CclsTestInit& orig) = delete;
    virtual ~CclsTestInit() = default;

    void result(const std::shared_ptr<psc::json::JsonValue>& result) override;
private:
    RpcLaunch* m_rpcLaunch;
    CclsTest* m_cclsTest;
    Glib::RefPtr<Gio::File> m_projDir;
};

class CclsTest
: public Gio::Application
{
public:
    CclsTest();
    explicit CclsTest(const CclsTest& orig) = delete;
    virtual ~CclsTest() = default;

    void on_activate() override;
    void initalized();
    bool getResult();
    void result();
    TextPos find(const Glib::ustring& text, Glib::UStringView find);

protected:
    void start();
    Glib::ustring json_build(const Glib::RefPtr<Gio::File>& file);
    Glib::ustring readText(const Glib::RefPtr<Gio::File>& file);
private:
    Glib::RefPtr<Glib::MainContext> m_mainContext;
    Glib::RefPtr<Glib::MainLoop> m_mainLoop;
    bool m_result{false};
    Glib::RefPtr<Gio::File> m_projDir;
    std::shared_ptr<CcLangServer> m_ccls;
};

