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
: LspConf::LspConf(config->getString(grp.c_str(), LANGUAGE_EXEC_KEY)
               , config->getString(grp.c_str(), LANGUAGE_EXTENSIONS_KEY)
               , config->getString(grp.c_str(), LANGUAGE_LSP_LANGUAGE_KEY)
               , config->getString(grp.c_str(), LANGUAGE_PREREQUISITEFILE_KEY))
{
}

LspConf::LspConf(const Glib::ustring& exec
            , const Glib::ustring& exts
            , const Glib::ustring& lspLanguage
            , const Glib::ustring& prerequisiteFile)
: m_execute{exec}
, m_lspLanguage{lspLanguage}
, m_prerequisiteFile{prerequisiteFile}
{
    std::vector<Glib::ustring> extensions;
    extensions.reserve(16);
    StringUtils::split(exts, EXTENSTION_DELIM_CHAR, extensions);
    for (auto ext : extensions) {
        m_extensions.emplace(std::move(ext));
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
    config->setString(grp.c_str(), LANGUAGE_EXTENSIONS_KEY, getExtensions());
    config->setString(grp.c_str(), LANGUAGE_LSP_LANGUAGE_KEY, getLspLanguage());
    config->setString(grp.c_str(), LANGUAGE_PREREQUISITEFILE_KEY, getPrerequisiteFile());
}

Glib::ustring
LspConf::getExecute()
{
    return m_execute;
}

Glib::ustring
LspConf::getExtensions()
{
    Glib::ustring ret;
    ret.reserve(32);
    auto delim = Glib::ustring(1, EXTENSTION_DELIM_CHAR);
    for (auto ext : m_extensions) {
        if (ret.size() > 0) {
            ret += delim;
        }
        ret += ext;
    }
    return ret;
}

Glib::ustring
LspConf::getLspLanguage()
{
    return m_lspLanguage;
}

Glib::ustring
LspConf::getPrerequisiteFile()
{
    return m_prerequisiteFile;
}

bool
LspConf::hasPrerequisite(const Glib::RefPtr<Gio::File>& file)
{
    if (!m_prerequisiteFile.empty()) {
        auto dir = file->get_parent();
        auto dotccls = dir->get_child(m_prerequisiteFile);
        return dotccls->query_exists();
    }
    return true;
}

bool
LspConf::hasExecutable()
{
    std::vector<Glib::ustring> parts;
    parts.reserve(8);
    StringUtils::split(m_execute, ' ', parts);
    auto file = Gio::File::create_for_path(parts[0]);
    return file->query_exists();
}

bool
LspConf::isExtension(const Glib::ustring& ext)
{
    return m_extensions.find(ext) != m_extensions.end();
}


LspConfs::LspConfs()
{
}

PtrLanguage
LspConfs::getLanguage(const Glib::RefPtr<Gio::File>& file)
{
    Glib::ustring ext = StringUtils::getExtension(file);
    PtrLanguage language;
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
        m_languages.push_back(createCcls());
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

PtrLanguage
LspConfs::createCcls()
{
    return std::make_shared<LspConf>("/usr/bin/ccls \"--init={\"\"index\"\":{\"\"threads\"\":1}}\"", "h,H,hpp,hh,hxx,c,C,cpp,c++,cc,cxx", "cpp", ".ccls");
}
