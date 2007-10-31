/*
 * $Id$
 *
 * Copyright (c) 2007 Vyacheslav Frolov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * $Log$
 * Revision 1.1  2007/10/31 10:16:55  vfrolov
 * Initial revision
 *
 *
 */

#pragma once

namespace SetupApp {
  using namespace System;
  using namespace System::Drawing;
  using namespace System::Windows::Forms;
  using namespace System::Collections::Generic;

  ref class PinSource;
  ref class Pin;
  ref class PortPair;

  ref class PinMap {
    public:
      PinMap();
      void Init(PortPair ^pair);
      void Paint(PaintEventArgs ^e, Control ^control);
      void GetChanges(PortPair ^pair);

      void MouseInit();
      void MouseDown(MouseEventArgs ^e);
      void MouseMove(MouseEventArgs ^e);
      void MouseUp(MouseEventArgs ^e);
      void MouseDoubleClick(MouseEventArgs ^e);

    private:
      void Link(String ^pinName, String ^source, bool invert);

      Dictionary<String ^, PinSource ^> ^pinsOrg;
      Dictionary<String ^, Pin ^> ^pins;
      Point mb;
      Point me;
      String ^selected;
      bool down;
      bool up;
      bool doubleClick;
  };
}
