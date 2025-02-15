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
#include <giomm.h>
#include <memory>

#include "ThreadWorker.hpp"


class WorkerStart
: public ThreadWorker<int, long>
{
public:
    WorkerStart() = default;
    explicit WorkerStart(const WorkerStart& orig) = delete;
    virtual ~WorkerStart() = default;

    virtual long doInBackground() override;
    virtual void process(const std::vector<int>& out) override;
    virtual void done() override;

    static constexpr int INTERM_VAL = 42;
    static constexpr long FINAL_VAL = 0xaa55aa55;
    int m_intermediate{};
};

class WorkerExcept
: public WorkerStart
{
public:
    WorkerExcept() = default;
    explicit WorkerExcept(const WorkerExcept& orig) = delete;
    virtual ~WorkerExcept() = default;

    long doInBackground() override;
    void done() override;

    bool m_error{false};
};

class TestApp
: public Gio::Application
{
public:
    TestApp();
    virtual ~TestApp() = default;

    void on_activate() override;
    int getResult();
protected:
    void start();

private:
    std::shared_ptr<WorkerStart> m_workerStart;
    std::shared_ptr<WorkerExcept> m_workerExcept;
};


