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
#include <chrono>
#include <thread>

#include "WorkerTest.hpp"
#include "VarselConfig.hpp"

using namespace std::chrono_literals; // ns, us, ms, s, h, etc.


TestApp::TestApp()
: Gio::Application("de.pfeifer_syscon.varselTest")
{
}

void
TestApp::start()
{
    m_workerStart = std::make_shared<WorkerStart>();
    m_workerStart->execute();
    m_workerExcept = std::make_shared<WorkerExcept>();
    m_workerExcept->execute();
}

void
TestApp::on_activate()
{
    // create main loop so the dispatch works
    auto main = Glib::MainLoop::create(false);
    main->get_context()->signal_idle().connect_once(
            sigc::mem_fun(*this, &TestApp::start));
    main->get_context()->signal_timeout().connect_seconds_once([&] {
            main->quit();
    }, 2);
    main->run();
}


bool
TestApp::match(const Gdk::RGBA& color, const Gdk::RGBA& color2)
{
    if (color.get_red_u() != color2.get_red_u()
     || color.get_green_u() != color2.get_green_u()
     || color.get_blue_u() != color2.get_blue_u()
     || color.get_alpha_u() != color2.get_alpha_u()) {
        std::cout << "color missmatch"
                  << " red " << color.get_red_u() << ", " << color2.get_red_u()
                  << " green " << color.get_green_u() << ", " << color2.get_green_u()
                  << " blue " << color.get_blue_u() << ", " << color2.get_blue_u()
                  << " alpha " << color.get_alpha_u() << ", " << color2.get_alpha_u() << std::endl;
        return false;
    }
    return true;
}

int
TestApp::getResult()
{

    if (!m_workerStart
     || !m_workerExcept) {
        std::cout << "The activation has failed -> check for hanging instances of this process!" << std::endl;
        return 9;
    }
    if (m_workerStart->m_intermediate != WorkerStart::INTERM_VAL) {
        std::cout << "Start expected result " << WorkerStart::INTERM_VAL << " got " << m_workerStart->m_intermediate << std::endl;
        return 1;
    }
    if (m_workerStart->getResult() != WorkerStart::FINAL_VAL) {
        std::cout << "Start expected end result " << std::hex << WorkerStart::FINAL_VAL << " got " << m_workerStart->getResult() << std::endl;
        return 2;
    }
    if (!m_workerExcept->m_error) {
        std::cout << "Except expected true got " << std::boolalpha << m_workerExcept->m_error << std::endl;
        return 3;
    }
    // Simply add the config test
    VarselConfig conf = VarselConfig("testing.conf");
    Gdk::RGBA color{"rgb(192,128,192)"};
    conf.setColor("abc", "color", color);
    Gdk::RGBA color2 = conf.getColor("abc", "color");
    if (!match(color, color2)) {
        return 4;
    }
    Gdk::RGBA color3{"rgba(192,192,128,0.345)"};
    conf.setColor("abc", "color2", color3);
    Gdk::RGBA color4 = conf.getColor("abc", "color2");
    if (!match(color3, color4)) {
        return 5;
    }

    return 0;
}

long
WorkerStart::doInBackground()
{
    std::this_thread::sleep_for(100ms);
    notify(INTERM_VAL);
    std::this_thread::sleep_for(100ms);

    return FINAL_VAL;
}

void
WorkerStart::process(const std::vector<int>& out)
{
    m_intermediate = *out.begin();
}

void
WorkerStart::done()
{
    try {
        getResult();
    }
    catch (const std::exception& exc) {
        std::cout << "WorkerStart::done exc " << exc.what() << std::endl;
    }
}

long
WorkerExcept::doInBackground()
{
    std::this_thread::sleep_for(100ms);
    throw std::invalid_argument( "expected exception" );
    return 0;
}


void
WorkerExcept::done()
{
    try {
        getResult();
    }
    catch (const std::exception& exc) {
        std::cout << "WorkerExcept::done " << exc.what() << std::endl;
        m_error = true;
    }
}

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();

    auto app = TestApp();
    app.run(argc, argv);
    return app.getResult();
}
