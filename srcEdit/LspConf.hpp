/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
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

#include <VarselConfig.hpp>
#include <memory>
#include <vector>
#include <set>
#include <glibmm.h>

class LspConf
{
public:
    LspConf(const std::shared_ptr<VarselConfig>& config
            , const Glib::ustring& grp);
    LspConf(const Glib::ustring& exec
            , const std::vector<Glib::ustring>& exts
            , const Glib::ustring& lspLanguage
            , const std::vector<Glib::ustring>& prerequisiteFile
            , bool indexingNeeded);
    explicit LspConf(const LspConf& orig) = delete;
    virtual ~LspConf() = default;

    bool isValid();
    void save(const std::shared_ptr<VarselConfig>& config, const Glib::ustring& grp);
    Glib::ustring getExecute();
    std::set<Glib::ustring> getExtensions();
    Glib::ustring getLspLanguage();
    std::vector<Glib::ustring> getPrerequisiteFile();

    bool isExtension(const Glib::ustring& ext);
    // checks if the local prerequisites are meet to use language e.g. .ccls file
    bool hasPrerequisite(const Glib::RefPtr<Gio::File>& file);
    // checks if the global prerequisites are meet to use language e.g. /usr/bin/ccls
    bool hasExecutable();
    bool isIndexingNeeded();
    static constexpr auto LANGUAGE_EXEC_KEY{"execute"};
    static constexpr auto LANGUAGE_EXTENSIONS_KEY{"extensions"};
    static constexpr auto LANGUAGE_LSP_LANGUAGE_KEY{"lspLanguage"};
    static constexpr auto LANGUAGE_PREREQUISITEFILE_KEY{"prerequisiteFile"};
    static constexpr auto LANGUAGE_INDEXING_REQUIRED{"requiresIndexing"};
protected:
private:
    Glib::ustring m_execute;
    std::set<Glib::ustring> m_extensions;
    Glib::ustring m_lspLanguage;
    std::vector<Glib::ustring> m_prerequisiteFile;
    bool m_indexingNeeded;
};

using PtrLspConf = std::shared_ptr<LspConf>;

class LspConfs
{
public:
    LspConfs();
    explicit LspConfs(const LspConfs& orig) = delete;
    virtual ~LspConfs() = default;

    PtrLspConf getLanguage(const Glib::RefPtr<Gio::File>& file);
    void readConfig(const std::shared_ptr<VarselConfig>& config);
    void saveConfig(const std::shared_ptr<VarselConfig>& config);
    // as a starting point ccls support is added
    static PtrLspConf createDefault();
protected:
    static constexpr auto MAX_LANGUAGES{100};
    static constexpr auto LANGUAGE_GPR{"language%03d"};
    static constexpr auto LSP_LANGUAGE_CPP{"cpp"};
    static std::vector<Glib::ustring>  createCsrc();
    static PtrLspConf createCcls();
    static PtrLspConf createClangd();


private:
    std::vector<PtrLspConf> m_languages;
};

