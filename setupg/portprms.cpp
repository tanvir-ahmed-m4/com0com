/*
 * $Id$
 *
 * Copyright (c) 2007-2009 Vyacheslav Frolov
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
 * Revision 1.2  2009/01/12 13:04:07  vfrolov
 * Added red painting InUse portnames
 *
 * Revision 1.1  2007/10/31 10:16:55  vfrolov
 * Initial revision
 *
 */

#include "stdafx.h"
#include "portprms.h"
#include "exec.h"

using namespace SetupApp;
using namespace System;
using namespace System::Collections::Generic;


PortParams::PortParams(String ^str)
{
  array<Char> ^separator = {','};
  array<String ^> ^prms = str->Split(separator, StringSplitOptions::RemoveEmptyEntries);;

  for (int i = 0 ; i < prms->Length ; i++) {
    array<Char> ^separator = {'='};
    array<String ^> ^prm = prms[i]->Split(separator);

    if (prm->Length == 2) {
      this[prm[0]->ToLower()] = prm[1]->ToUpper();
    }
  }
}

String ^PortPairs::ParseLine(String ^line)
{
  array<Char> ^separator = {' '};
  array<String ^> ^fields = line->Split(separator, StringSplitOptions::RemoveEmptyEntries);

  if (fields->Length != 2)
    return nullptr;

  int iPort;

  if (fields[0]->StartsWith("CNCA")) {
    iPort = 0;
  }
  else
  if (fields[0]->StartsWith("CNCB")) {
    iPort = 1;
  }
  else {
    return nullptr;
  }

  String ^keyPair = fields[0]->Substring(4);

  if (!ContainsKey(keyPair))
    this[keyPair] = gcnew PortPair();

  this[keyPair]->Set(iPort, gcnew PortParams(fields[1]));

  return keyPair;
}

void PortPairs::Init()
{
  String ^cmd = "--detail-prms list";

  array<String ^> ^lines = ExecCommand::ExecCommand(parent, cmd);

  Clear();

  for (int i = 0 ; i < lines->Length ; i++)
    ParseLine(lines[i]);

  LoadBusyNames();
}

String ^PortPairs::AddPair()
{
  String ^res = nullptr;

  String ^cmd = "--detail-prms install - -";

  array<String ^> ^lines = ExecCommand::ExecCommand(parent, cmd);

  for (int i = 0 ; i < lines->Length ; i++) {
    String ^keyPair = ParseLine(lines[i]);

    if (res == nullptr)
      res = keyPair;
  }

  LoadBusyNames();

  return res;
}

void PortPairs::RemovePair(String ^keyPair)
{
  String ^cmd = String::Format("remove {0}", keyPair);

  ExecCommand::ExecCommand(parent, cmd);

  Init();
}

void PortPairs::ChangePair(String ^keyPair, PortPair ^pairChanges)
{
  for (int i = 0 ; i < 2 ; i++) {
    if (pairChanges[i] == nullptr || pairChanges[i]->Count == 0)
      continue;

    String ^cmd = "change";
    cmd += String::Format(" CNC{0}{1} ", (i == 0) ? "A" : "B", keyPair);

    for each (KeyValuePair<String ^, String ^> kvpPort in pairChanges[i])
      cmd += String::Format("{0}={1},", kvpPort.Key, kvpPort.Value);

    ExecCommand::ExecCommand(parent, cmd);
  }

  Init();
}

bool PortPairs::IsValidName(String ^name)
{
  if (busyNames == nullptr)
    return true;

  for each (String ^busyName in busyNames) {
    if (busyName->ToUpper() == name->ToUpper())
      return false;
  }

  return true;
}

void PortPairs::LoadBusyNames()
{
  String ^cmd = "busynames *";

  busyNames = ExecCommand::ExecCommand(parent, cmd);
}
