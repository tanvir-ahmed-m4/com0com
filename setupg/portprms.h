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
  using namespace System::Collections::Generic;

  ref class PortParams : public Dictionary<String ^, String ^> {
    public:
      PortParams() {}
      PortParams(String ^str);
  };

  ref class PortPair {
    public:
      PortPair() { ports = gcnew array<PortParams ^>{gcnew PortParams, gcnew PortParams}; }

      PortParams ^operator [](int i) { return ports[i]; }
      void Set(int i, PortParams ^port) { ports[i] = port; }
      bool IsEmpty() { return ports[0]->Count == 0 && ports[1]->Count == 0; }

    private:
      array<PortParams ^> ^ports;
  };

  ref class PortPairs : public Dictionary<String ^, PortPair ^> {
    public:
      PortPairs(System::Windows::Forms::Control ^_parent) : parent(_parent) {}
      void Init();
      String ^AddPair();
      void RemovePair(String ^keyPair);
      void ChangePair(String ^keyPair, PortPair ^pairChanges);

    private:
      String ^ParseLine(String ^line);

      System::Windows::Forms::Control ^parent;
  };
}
