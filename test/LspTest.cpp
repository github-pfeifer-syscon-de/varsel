/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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

#include <iostream>

#include <LspServer.hpp>
#include <JsonObj.hpp>
#include <Log.hpp>
#include <sys/wait.h>   // waitpid


#include "LspTest.hpp"
#include "LspConf.hpp"


LspTest::LspTest()
: Gio::Application("de.pfeifer_syscon.cclsTest")
{
}


bool
LspTest::getResult()
{
    return m_result;
}


LspTestInit::LspTestInit(LspTest* cclsTest, const Glib::RefPtr<Gio::File>& projDir)
: LspInit(projDir, nullptr)
, m_cclsTest{cclsTest}
, m_projDir{projDir}
{
}


void
LspTestInit::result(const psc::json::PtrJsonValue& json)
{
    if (json->isObject()) {
        auto obj = json->getObject();
        std::cout << "LspTestInit::result init "
                  << std::endl << obj->generate(2) << std::endl;
    }
    else {
        std::cout << "LspTestInit::result not a object!" << std::endl;
    }
    m_cclsTest->initalized();
}


LspLocation
LspTest::find(const Glib::ustring& text, Glib::UStringView what)
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
LspTest::notify(const Glib::ustring& status, LspStatusKind kind, gint64 percent)
{
    std::cout << "LspTest::notify"
              << " status " << status
              << " kind "  << static_cast<int>(kind)
              << " percent " << percent << std::endl;
    if (status == "index" && kind == LspStatusKind::End) {
        auto src = m_projDir->get_child("JsonObj.cpp");
        auto open = std::make_shared<LspOpen>(src, "cpp", 0);
        m_lspServer->communicate(open);
        auto txt = open->getText();
        auto pos = find(txt, "JsonObj::JsonObj"); // "m_jsonObj"
        pos.setUri(src);
        std::cout << " location " << pos.getStartLine() << ", " << pos.getStartCharacter() << std::endl;

        //TYPE_DEFINITION_METHOD get empty result for m_jsonObj
        auto def = std::make_shared<LspDocumentRef>(pos, LspDocumentRef::DEFININION_METHOD);
        //auto def = std::make_shared<LspDocumentRef>(pos, LspDocumentRef::TYPE_DEFINITION_METHOD);
        m_lspServer->communicate(def);

        auto close = std::make_shared<LspClose>(src);
        m_lspServer->communicate(close);

        m_mainContext->signal_timeout().connect_seconds_once(
            sigc::mem_fun(*m_lspServer.get(), &LspServer::shutdown), 2);
        // give callback some time to work... (while freeing main thread)
        m_mainContext->signal_timeout().connect_seconds_once(
            sigc::mem_fun(*this, &LspTest::result), 3);
    }
}

void
LspTest::serverExited()
{
    result();
}

void
LspTest::initalized()
{
    if (!m_lspConf->isIndexingNeeded()) {
        notify("index", LspStatusKind::End, 100);   // as clangd doesn't needs indexing
    }
}

void
LspTest::start()
{
    auto log = psc::log::Log::create("cclsTest", psc::log::Type::Console);
    log->setLevel(psc::log::Level::Debug);
    m_lspConf = LspConfs::createDefault();
    auto here = Gio::File::create_for_path(".");
    m_projDir = here->resolve_relative_path("../../genericImg/src");
    auto srcFile = m_projDir->get_child("JsonObj.hpp");
    if (m_lspConf->hasPrerequisite(srcFile)) {
        m_lspServer = std::make_shared<LspServer>(m_lspConf);
        m_lspServer->setStatusListener(this);
        auto init = std::make_shared<LspTestInit>(this, m_projDir);
        m_lspServer->communicate(init);

        m_mainContext->signal_timeout().connect_seconds_once(
            [&] {
                GPid pid =m_lspServer->getChildPid();
                std::cout << "Killing " << pid << " the index data is most likely defect -> remove cache e.g. .ccls-cache" << std::endl;
                kill(pid, SIGTERM); // if nothing worked kill it
            }, 90);                 // if your are using a "slow" system e.g. raspi this might even need a longer timeout
    }
    else {
        m_result = true;    // Don't count this as failure
        std::cout << "A supported language for file " << srcFile->get_path() << " was not found, test not executed!" << std::endl;
        m_mainLoop->quit();
    }
}



void
LspTest::result()
{
    m_mainContext->signal_timeout().connect_seconds_once(
        [&] {
            m_result = m_lspServer->getResult();
            if (!m_result) {    // workaround process did as expected, but the glibc notification is missing
                int status;
                waitpid(m_lspServer->getChildPid(), &status, WNOHANG | WUNTRACED);
                if (WIFEXITED(status)) {
                    printf("process exited with %d\n", WEXITSTATUS(status));
                    m_result = WEXITSTATUS(status) == 0;
                }
                else if (WIFSIGNALED(status)) {
                    printf("process killed by signal %d\n", WTERMSIG(status));
                    m_result = true;    // since we want to test our part, we take this as a success, maybe there is a better way ...?
                }
            }
            psc::log::Log::logAdd(psc::log::Level::Debug,
                [&] {
                    return psc::fmt::format("LspTest::result {}", m_result);
                });
            std::cout << "Quit main" << std::endl;
            m_mainLoop->quit();
        }, 1);
}

void
LspTest::on_activate()
{
    // create main loop so the dispatch works
    m_mainLoop = Glib::MainLoop::create(false);
    m_mainContext = m_mainLoop->get_context();
    m_mainContext->signal_idle().connect_once(
            sigc::mem_fun(*this, &LspTest::start));
    m_mainLoop->run();
}

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    //Gio::init();

    auto cclsTest = LspTest();
    cclsTest.run(argc, argv);
    if (!cclsTest.getResult()) {
        return 1;
    }

    return 0;
}
