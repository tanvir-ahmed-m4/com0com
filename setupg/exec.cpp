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

#include "stdafx.h"
#include "exec.h"

using namespace System;
using namespace System::Windows::Forms;

array<String ^> ^ExecCommand::ExecCommand(System::Windows::Forms::Control ^parent, String ^cmd)
{
  Cursor ^cursor = parent->Cursor;
  parent->Cursor = Cursors::WaitCursor;

  System::Diagnostics::Process ^setupc = gcnew System::Diagnostics::Process();
  setupc->StartInfo->FileName = "setupc.exe";

  setupc->StartInfo->Arguments = cmd->Trim();

  setupc->StartInfo->CreateNoWindow = true;
  setupc->StartInfo->UseShellExecute = false;
  setupc->StartInfo->RedirectStandardOutput = true;

  array<String ^> ^lines = gcnew array<String ^>(0);

  try {
    if (setupc->Start()) {
      for (;;) {
        String ^line = setupc->StandardOutput->ReadLine();
        if (!line)
          break;

        Array::Resize(lines, lines->Length + 1);
        lines[lines->Length - 1] = line->Trim();
      }

      setupc->WaitForExit();

      /*
      String ^msg = String::Format("\"{0} {1}\"\n\n{2}",
                                   setupc->StartInfo->FileName,
                                   setupc->StartInfo->Arguments,
                                   String::Join("\n", lines));

      MessageBox::Show(msg, "");
      */

      setupc->Close();
    }
  }
  catch (Exception ^e) {
    String ^msg = String::Format("\"{0} {1}\"\n\n{2}",
                                 setupc->StartInfo->FileName,
                                 setupc->StartInfo->Arguments,
                                 e->Message);
    MessageBox::Show(msg, "Error");
  }

  parent->Cursor = cursor;

  return lines;
}
