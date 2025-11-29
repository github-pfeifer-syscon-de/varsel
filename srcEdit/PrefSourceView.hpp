/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
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

#pragma once

class SourceView;

class PrefSourceView
: public Gtk::Dialog
{
public:
    PrefSourceView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, SourceView* parent);
    explicit PrefSourceView(const PrefSourceView& orig) = delete;
    virtual ~PrefSourceView() = default;

    void on_sourcestyle_select();
    void on_response(int response) override;

    static constexpr auto CONFIG_SRCVIEW_GRP = "sourceView";
    static constexpr auto SOURCE_STYLE = "sourceStyle";
    static constexpr auto CONFIG_FONT = "font";
    static constexpr auto DEFAULT_FONT = "defaultFont";

private:
    Gtk::CheckButton* m_defaultFont;
    Gtk::FontButton* m_fontButton;
    Gtk::ComboBoxText* m_sourceStyle;
    SourceView* m_parent;
};

