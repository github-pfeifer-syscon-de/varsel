/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4 -*-  */
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

#include <iostream>
#include <glibmm.h>
#include <future>
#include <chrono>
#include <queue>
#include <vector>



template< typename T >
class ConcurrentQueue
{

    using const_iterator = typename std::deque<T>::const_iterator;

public:

    ConcurrentQueue()
    {
    }



    /** @brief Emplaces a new instance of T at the back of the deque **/
    template<typename... Args>
    void emplace_back( Args&&... args )
    {
        addData_protected( [&] {
            m_collection.emplace_back(std::forward<Args>(args)...);
        } );
    }

    template<typename... Args>
    void push_back( Args&&... args )
    {
        addData_protected( [&] {
            m_collection.push_back(std::forward<Args>(args)...);
        } );
    }

    /** @brief  Returns the the queued elements with out.
    **/
    bool pop_front(std::vector<T>& out) noexcept
    {
        if (!m_collection.empty()) {
            std::unique_lock<std::mutex> lock{m_mutex};
            out.reserve(m_collection.size());
            while(!m_collection.empty()) {
               out.emplace_back(std::move(m_collection.front()));
               m_collection.pop_front();
            }
            lock.unlock();
        }
        return m_active;
    }

    void finish()
    {
        m_active = false;
    }

private:

    /** @brief  Protects the deque, calls the provided function and notifies the presence of new data
        @param  The concrete operation to be used. It MUST be an operation which will add data to the deque,
                as it will notify that new data are available!
    **/
    template<class F>
    void addData_protected(F&& fct)
    {
        std::unique_lock<std::mutex> lock{ m_mutex };
        fct();
        lock.unlock();
    }

    std::deque<T> m_collection;              ///< Concrete, not thread safe, storage.

    std::mutex   m_mutex;                    ///< Mutex protecting the concrete storage

    bool m_active{true};

};

template <typename I, typename T>
class ThreadWorker
{
public:
    ThreadWorker()
    {
        // to avoid inconsistent state of queue/completion use single notification
        m_notify.connect(
                sigc::mem_fun(*this, &ThreadWorker::emited));
    }
    ThreadWorker(const ThreadWorker& orig) = default;
    virtual ~ThreadWorker() = default;

    void execute()
    {
        m_future = std::async(std::launch::async, &ThreadWorker::run, this);
    }
    T run()
    {
        T t;
        try {
            //std::cout << "ThreadWorker::run " << this << std::endl;
            t = doInBackground();
        }
        catch (...) {
            m_eptr = std::current_exception();
        }
        //std::cout << "Thread::run done" << std::endl;
        m_queue.finish();
        // ensure sequential execution for last step
        std::unique_lock<std::mutex> lock{ m_mutex };
        emit();
        lock.unlock();
        return t;
    }
    void emit()
    {
        if (!m_pending) {
            m_pending = true;
            m_notify.emit();    // ensure activation (in any case)
        }
    }
    void notify(I i) {
        //std::cout << "ThreadWorker::notify " << std::boolalpha << m_queue.isActive() << std::endl;
        m_queue.emplace_back(i);
        emit();
    }
    void emited()
    {
        std::unique_lock<std::mutex> lock{ m_mutex };
        std::vector<I> out;
        bool active = m_queue.pop_front(out);
        //std::cout << "ThreadWorker::emited active " << std::boolalpha << active << std::endl;
        if (!out.empty()) {
            process(out);
        }
        m_pending = false;
        if (!active && !m_completed) {
            m_completed = true;
            completed();
        }
        lock.unlock();
        //std::cout << "ThreadWorker::emited done " << std::boolalpha << m_queue.isActive() << std::endl;
    }
    void completed()
    {
        m_t = m_future.get();
        done();
    }
    // this function propagates the exception
    //   that has been thrown when executing doInBackground
    //   or the result
    T getResult()
    {
        if (m_eptr) {
            std::rethrow_exception(m_eptr);
        }
        return m_t;
    }

protected:
    virtual T doInBackground() = 0;
    virtual void process(const std::vector<I>& out) = 0;
    virtual void done() = 0;
private:
    ConcurrentQueue<I> m_queue;
    Glib::Dispatcher m_notify;
    std::future<T> m_future;
    T m_t;
    std::exception_ptr m_eptr;
    volatile bool m_completed{false};
    volatile bool m_pending{false};
    std::mutex m_mutex;
};

