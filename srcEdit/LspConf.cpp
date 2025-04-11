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

#include <StringUtils.hpp>

#include "LspConf.hpp"

LspConf::LspConf(const std::shared_ptr<VarselConfig>& config, const Glib::ustring& grp)
: LspConf(config->getString(grp.c_str(), LANGUAGE_EXEC_KEY)
               , config->getStringList(grp.c_str(), LANGUAGE_EXTENSIONS_KEY)
               , config->getString(grp.c_str(), LANGUAGE_LSP_LANGUAGE_KEY)
               , config->getStringList(grp.c_str(), LANGUAGE_PREREQUISITEFILE_KEY)
               , config->getBoolean(grp.c_str(), LANGUAGE_INDEXING_REQUIRED, false))
{
}

LspConf::LspConf(const Glib::ustring& exec
            , const std::vector<Glib::ustring>& exts
            , const Glib::ustring& lspLanguage
            , const std::vector<Glib::ustring>& prerequisiteFile
            , bool needsIndexing)
: m_execute{exec}
, m_lspLanguage{lspLanguage}
, m_prerequisiteFile{prerequisiteFile}
, m_indexingNeeded{needsIndexing}
{
    for (auto& ext : exts) {
        m_extensions.insert(ext);
    }
}

bool
LspConf::isValid()
{
    return !m_execute.empty() && !m_extensions.empty() && !m_lspLanguage.empty();
}

void
LspConf::save(const std::shared_ptr<VarselConfig>& config, const Glib::ustring& grp)
{
    config->setString(grp.c_str(), LANGUAGE_EXEC_KEY, getExecute());
    config->setStringList(grp.c_str(), LANGUAGE_EXTENSIONS_KEY, getExtensions());
    config->setString(grp.c_str(), LANGUAGE_LSP_LANGUAGE_KEY, getLspLanguage());
    config->setStringList(grp.c_str(), LANGUAGE_PREREQUISITEFILE_KEY, getPrerequisiteFile());
    config->setBoolean(grp.c_str(), LANGUAGE_INDEXING_REQUIRED, isIndexingNeeded());
}

Glib::ustring
LspConf::getExecute()
{
    return m_execute;
}

std::set<Glib::ustring>
LspConf::getExtensions()
{
    return m_extensions;
}

Glib::ustring
LspConf::getLspLanguage()
{
    return m_lspLanguage;
}

std::vector<Glib::ustring>
LspConf::getPrerequisiteFile()
{
    return m_prerequisiteFile;
}

bool
LspConf::hasPrerequisite(const Glib::RefPtr<Gio::File>& file)
{
    auto dir = file->get_parent();
    if (!m_prerequisiteFile.empty()) {
        for (auto& part : m_prerequisiteFile) {
            auto preqFile = dir->get_child(part);
            //std::cout << "LspConf::hasPrerequisite check " << preqFile->get_basename()
            //          << " exists " << std::boolalpha << preqFile->query_exists() << std::endl;
            if (preqFile->query_exists()) {
                return true;
            }
        }
        return false;
    }
    return true;
}

bool
LspConf::hasExecutable()
{
    std::vector<Glib::ustring> parts = StringUtils::splitQuoted(m_execute, ' ', '\"');
    auto file = Gio::File::create_for_path(parts[0]);
    return file->query_exists();
}

bool
LspConf::isIndexingNeeded()
{
    return m_indexingNeeded;
}

bool
LspConf::isExtension(const Glib::ustring& ext)
{
    return m_extensions.find(ext) != m_extensions.end();
}


LspConfs::LspConfs()
{
}

PtrLspConf
LspConfs::getLanguage(const Glib::RefPtr<Gio::File>& file)
{
    Glib::ustring ext = StringUtils::getExtension(file);
    PtrLspConf language;
    for (auto& lang : m_languages) {
        if (lang->isExtension(ext)
         && lang->hasPrerequisite(file)
         && lang->hasExecutable()) {
            language = lang;
            break;
        }
    }
    return language;
}

void
LspConfs::readConfig(const std::shared_ptr<VarselConfig>& config)
{
    m_languages.reserve(16);
    for (uint32_t i = 0; i < MAX_LANGUAGES; ++i) {
        Glib::ustring grp = Glib::ustring::sprintf(LANGUAGE_GPR, i);
        if (config->hasGrp(grp.c_str())) {
            auto langServ = std::make_shared<LspConf>(config, grp);
            if (langServ->isValid()) {
                m_languages.emplace_back(std::move(langServ));
            }
        }
    }
    if (m_languages.empty()) {
        m_languages.push_back(createDefault());
    }
}

void
LspConfs::saveConfig(const std::shared_ptr<VarselConfig>& config)
{
    uint32_t i = 0;
    for (auto& lang : m_languages) {
        Glib::ustring grp = Glib::ustring::sprintf(LANGUAGE_GPR, i);
        lang->save(config, grp);
        ++i;
    }
}

std::vector<Glib::ustring>
LspConfs::createCsrc()
{
    std::vector<Glib::ustring> exts{"h","H","hpp","hh","hxx","c","C","cpp","c++","cc","cxx",};
    return exts;
}

PtrLspConf
LspConfs::createCcls()
{
    std::vector<Glib::ustring> exts = createCsrc();
    std::vector<Glib::ustring> reqs{".ccls", "compile_commands.json"};
    // - threads:1 was selected as otherwise there seem to be issues to complete indexing
    return std::make_shared<LspConf>(
              "/usr/bin/ccls \"--init={\"\"index\"\":{\"\"threads\"\":1}}\""
            , exts
            , LSP_LANGUAGE_CPP
            , reqs
            , true);

}

PtrLspConf
LspConfs::createClangd()
{
    auto exts = createCsrc();
    std::vector<Glib::ustring> reqs{"compile_commands.json"};
    return std::make_shared<LspConf>(
            "/usr/bin/clangd"
            , exts
            , LSP_LANGUAGE_CPP
            , reqs
            , false);
}

PtrLspConf
LspConfs::createDefault()
{
    return createClangd();
}
