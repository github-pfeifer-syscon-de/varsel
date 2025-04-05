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

#include "Language.hpp"

Languages::Languages()
{
}

PtrLanguage
Languages::getLanguage(const Glib::RefPtr<Gio::File>& file)
{
    Glib::ustring ext = StringUtils::getExtension(file);
    PtrLanguage language;
    for (auto& lang : m_languages) {
        if (lang->isExtension(ext)
         && lang->hasPrerequisite(file)) {
            language = lang;
            break;
        }
    }
    return language;
}

void
Languages::readConfig(const std::shared_ptr<VarselConfig>& config)
{
    m_languages.reserve(16);
    for (uint32_t i = 0; i < MAX_LANGUAGES; ++i) {
        Glib::ustring grp = Glib::ustring::sprintf(LANGUAGE_GPR, i);
        if (config->hasGrp(grp.c_str())) {
            auto exec = config->getString(grp.c_str(), LANGUAGE_EXEC_KEY);
            auto exts = config->getString(grp.c_str(), LANGUAGE_EXTENSIONS_KEY);
            auto lang = config->getString(grp.c_str(), LANGUAGE_LSP_LANGUAGE_KEY);
            auto preReq = config->getString(grp.c_str(), LANGUAGE_PREREQUISITEFILE_KEY);
            if (!exec.empty() && !exts.empty() && !lang.empty()) {
                auto langServ = std::make_shared<Language>(exec, exts, lang, preReq);
                m_languages.emplace_back(std::move(langServ));
            }
        }
    }
    if (m_languages.empty()) {
        m_languages.push_back(createCcls());
    }
}

void
Languages::saveConfig(const std::shared_ptr<VarselConfig>& config)
{
    uint32_t i = 0;
    for (auto& lang : m_languages) {
        Glib::ustring grp = Glib::ustring::sprintf(LANGUAGE_GPR, i);
        config->setString(grp.c_str(), LANGUAGE_EXEC_KEY, lang->getExecute());
        config->setString(grp.c_str(), LANGUAGE_EXTENSIONS_KEY, lang->getExtensions());
        config->setString(grp.c_str(), LANGUAGE_LSP_LANGUAGE_KEY, lang->getLspLanguage());
        config->setString(grp.c_str(), LANGUAGE_PREREQUISITEFILE_KEY, lang->getPrerequisiteFile());
        ++i;
    }
}

PtrLanguage
Languages::createCcls()
{
    return std::make_shared<Language>("/usr/bin/ccls", "h,H,hpp,hh,hxx,c,C,cpp,c++,cc,cxx", "cpp", ".ccls");
}

Language::Language(
            const Glib::ustring& exec
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

Glib::ustring
Language::getExecute()
{
    return m_execute;
}

Glib::ustring
Language::getExtensions()
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
Language::getLspLanguage()
{
    return m_lspLanguage;
}

Glib::ustring
Language::getPrerequisiteFile()
{
    return m_prerequisiteFile;
}

bool
Language::hasPrerequisite(const Glib::RefPtr<Gio::File>& file)
{
    if (!m_prerequisiteFile.empty()) {
        auto dir = file->get_parent();
        auto dotccls = dir->get_child(m_prerequisiteFile);
        return dotccls->query_exists();
    }
    return true;
}

bool
Language::isExtension(const Glib::ustring& ext)
{
    return m_extensions.find(ext) != m_extensions.end();
}
