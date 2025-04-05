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

#include <LspServer.hpp>
#include <JsonObj.hpp>
#include <Log.hpp>

#include "CclsTest.hpp"
#include "Language.hpp"


CclsTest::CclsTest()
: Gio::Application("de.pfeifer_syscon.cclsTest")
{
}


bool
CclsTest::getResult()
{
    return m_result;
}


CclsTestInit::CclsTestInit(CclsTest* cclsTest, const Glib::RefPtr<Gio::File>& projDir)
: LspInit(projDir, nullptr)
, m_cclsTest{cclsTest}
, m_projDir{projDir}
{
}


void
CclsTestInit::result(const psc::json::PtrJsonValue& json)
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


LspLocation
CclsTest::find(const Glib::ustring& text, Glib::UStringView what)
{
    LspLocation tpos;
    auto pos = text.find(what.c_str());
    if (pos != text.npos) {
        --pos;
        auto nl = text.rfind('\n', pos);
        if (nl != text.npos) {
            tpos.setStartCharacter(pos - nl);
        }
        nl = pos;
        while (nl != text.npos && nl > 0) {
            nl = text.rfind('\n', nl-1);
            tpos.setStartLine(tpos.getStartLine() + 1);
        }
    }
    return tpos;
}

void
CclsTest::notify(const Glib::ustring& status, CclsStatusKind kind, gint64 percent)
{
    std::cout << "CclsTest::notify"
              << " status " << status
              << " kind "  << static_cast<int>(kind)
              << " percent " << percent << std::endl;
    if (status == "index" && kind == CclsStatusKind::End) {
        auto src = m_projDir->get_child("JsonObj.cpp");
        auto open = std::make_shared<LspOpen>(src, "cpp", 0);
        m_ccls->communicate(open);
        auto txt = open->getText();
        auto pos = find(txt, "JsonObj::JsonObj");
        pos.setUri(src);
        std::cout << " location " << pos.getStartLine() << ", " << pos.getStartCharacter() << std::endl;

        auto def = std::make_shared<LspDocumentRef>(pos, LspDocumentRef::DEFININION_METHOD);
        m_ccls->communicate(def);

        auto close = std::make_shared<LspClose>(src);
        m_ccls->communicate(close);

        m_mainContext->signal_timeout().connect_seconds_once(
            sigc::mem_fun(*m_ccls.get(), &LspServer::shutdown), 2);
        // give callback some time to work... (while freeing main thread)
        m_mainContext->signal_timeout().connect_seconds_once(
            sigc::mem_fun(*this, &CclsTest::result), 3);
    }
}

void
CclsTest::serverExited()
{
    result();
}

void
CclsTest::initalized()
{
}

void
CclsTest::start()
{
    auto log = psc::log::Log::create("cclsTest", psc::log::Type::Console);
    log->setLevel(psc::log::Level::Debug);
    auto lang = Languages::createCcls();
    auto here = Gio::File::create_for_path(".");
    m_projDir = here->resolve_relative_path("../../genericImg/src");
    auto srcFile = m_projDir->get_child("JsonObj.hpp");
    if (lang->hasPrerequisite(srcFile)) {
        m_ccls = std::make_shared<LspServer>(lang); // "/usr/bin/ccls --log-file=/tmp/ccls.log --init={\"index\":{\"threads\":1}}"
        m_ccls->setStatusListener(this);
        auto init = std::make_shared<CclsTestInit>(this, m_projDir);
        m_ccls->communicate(init);

        m_mainContext->signal_timeout().connect_seconds_once(
            [&] {
                GPid pid =m_ccls->getChildPid();
                std::cout << "Killing " << pid << " the index data is most likely defect -> remove .ccls-cache" << std::endl;
                kill(pid, SIGTERM); // if nothing worked kill it
            }, 90);                 // if your are using a "slow" system e.g. raspi this might event need a longer timeout
    }
    else {
        m_result = true;    // Don't count this as failure
        std::cout << "A supported language for file " << srcFile->get_path() << " was not found, test not executed!" << std::endl;
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
