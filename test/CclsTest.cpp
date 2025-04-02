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

#include <iostream>

#include <CCLangServer.hpp>
#include <JsonObj.hpp>
#include <Log.hpp>

#include "CclsTest.hpp"



CclsTest::CclsTest()
: Gio::Application("de.pfeifer_syscon.cclsTest")
{
}


bool
CclsTest::getResult()
{
    return m_result;
}


CclsTestInit::CclsTestInit(RpcLaunch* rpcLaunch, CclsTest* cclsTest, const Glib::RefPtr<Gio::File>& projDir)
: CclsInit(projDir)
, m_rpcLaunch{rpcLaunch}
, m_cclsTest{cclsTest}
, m_projDir{projDir}
{
}


void
CclsTestInit::result(const std::shared_ptr<psc::json::JsonValue>& json)
{
    if (json->isObject()) {
        auto obj = json->getObject();
        std::cout << "RpcInit::result init "
                  << std::endl << obj->generate(2) << std::endl;
    }
    else {
        std::cout << "CclsTestInit::result not a object!" << std::endl;
    }
    m_cclsTest->initalized();
}


TextPos
CclsTest::find(const Glib::ustring& text, Glib::UStringView what)
{
    TextPos tpos{0, 0};
    auto pos = text.find(what.c_str());
    if (pos != text.npos) {
        --pos;
        auto nl = text.rfind('\n', pos);
        if (nl != text.npos) {
            tpos.character = pos - nl;
        }
        nl = pos;
        do {
            nl = text.rfind('\n', nl-1);
            ++tpos.line;
        }
        while (nl != text.npos);
    }
    return tpos;
}

void
CclsTest::initalized()
{
    auto src = m_projDir->get_child("JsonObj.cpp");
    auto open = std::make_shared<CclsOpen>(src, "cpp", 0);
    m_ccls->communicate(open);
    auto txt = open->getText();
    auto pos = find(txt, "JsonObj::JsonObj");
    std::cout << "line " << pos.line << " column " << pos.character << std::endl;
    m_mainContext->signal_timeout().connect_seconds_once([&,src,pos] {
        auto def = std::make_shared<CclsDefinition>(src, pos);
        m_ccls->communicate(def);
    }, 5);
    m_mainContext->signal_timeout().connect_seconds_once([&,src] {
        auto close = std::make_shared<CclsClose>(src);
        m_ccls->communicate(close);
    }, 6);
}


void
CclsTest::start()
{
    auto log = psc::log::Log::create("cclsTest", psc::log::Type::Console);
    log->setLevel(psc::log::Level::Debug);
    m_ccls = std::make_shared<CcLangServer>();
    auto here = Gio::File::create_for_path(".");
    m_projDir = here->resolve_relative_path("../../genericImg/src");
    auto dotccls = m_projDir->get_child(".ccls");
    if (dotccls->query_exists()) {
        auto init = std::make_shared<CclsTestInit>(m_ccls.get(), this, m_projDir);
        m_ccls->communicate(init);

        m_mainContext->signal_timeout().connect_seconds_once(
            sigc::mem_fun(*m_ccls.get(), &CcLangServer::shutdown), 24);
        // give callback some time to work... (while freeing main thread)
        m_mainContext->signal_timeout().connect_seconds_once(
            sigc::mem_fun(*this, &CclsTest::result), 25);
    }
    else {
        m_result = true;    // Don't count this as failure
        std::cout << "The file " << dotccls->get_path() << " was not found, test not executed!" << std::endl;
        m_mainLoop->quit();
    }
}



void
CclsTest::result()
{
    std::cout << "CclsTest::result" << std::endl;
    m_result = m_ccls->getResult();
    std::cout << "Result " << std::boolalpha << m_result << std::endl;
    m_mainContext->signal_timeout().connect_seconds_once([&] {
        std::cout << "Quit main" << std::endl;
        m_mainLoop->quit();
    }, 1);
    //auto genImg = Gio::File::create_for_path("/home/rpf/csrc.git/genericImg");
    //auto json = json_build(genImg);
    //std::cout << "json " << json << std::endl;
    //m_result = (json == "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"initialize\",\"params\":{\"processId\":0,\"rootUri\":\"file:///home/rpf/csrc.git/genericImg\",\"capabilities\":{}}}");
}

void
CclsTest::on_activate()
{
    // create main loop so the dispatch works
    m_mainLoop = Glib::MainLoop::create(false);
    m_mainContext = m_mainLoop->get_context();
    m_mainContext->signal_idle().connect_once(
            sigc::mem_fun(*this, &CclsTest::start));
    m_mainLoop->run();
}

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    //Gio::init();

    auto cclsTest = CclsTest();
    cclsTest.run(argc, argv);
    if (!cclsTest.getResult()) {
        return 1;
    }

    return 0;
}
