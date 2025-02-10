/* -*- Mode: C; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2020 rpf
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

#include <gtkmm.h>

class VarselWin;

/*
 * dialog to change the font used for text display.
 */
class PrefDialog : public Gtk::Dialog {
public:
    PrefDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder, VarselWin* parent);
    virtual ~PrefDialog() = default;

    void on_sourcestyle_select();
    void on_response(int response) override;
    void on_font_select();

    static constexpr auto CONFIG_FONT = "font";
    //static constexpr auto CONFIG_FONT_SIZE = "fontSize";
    static constexpr auto DEFAULT_FONT = "defaultFont";
    static constexpr auto SOURCE_STYLE = "sourceStyle";

private:
    Gtk::FontButton* m_fontButton;
    Gtk::CheckButton* m_defaultFont;
    Gtk::ComboBoxText* m_sourceStyle;
    VarselWin* m_parent;

};

