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
#include "portprms.h"
#include "pinmap.h"

namespace SetupApp {
  using namespace System;

  ref class PinSource {
    public:

      PinSource(String ^_source, bool _invert) : source(_source), invert(_invert) {}
      PinSource(PinSource ^pin) : source(pin->source), invert(pin->invert) {}
      void Invert() { invert = !invert; }
      bool IsEqual(PinSource ^pin) {
        return source == pin->source && invert == pin->invert;
      }

      String ^source;
      bool invert;
  };

  ref class Pin {
    public:

      Pin(int c, int l, bool _isFixed, int ic)
        : C(c),
          L(l),
          IC(ic),
          isFixed(_isFixed),
          source(nullptr) {}

      void Link(PinSource ^_source) { source = gcnew PinSource(_source); }
      bool IsSource() { return source == nullptr; }
      void Invert() {
        if (source != nullptr)
          source->Invert();
      }

      int C;
      int L;
      int IC;
      bool isFixed;

      PinSource ^source;
  };
}

using namespace SetupApp;
using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

#define NUM_LINES          10
#define NUM_MIDDLE_SRCS    1
#define MAX_DIFF           6

#define UL                 0
#define DL                 (NUM_LINES - 1)

#define NUM_COLUMS         ((MAX_DIFF + 1)*2 + NUM_MIDDLE_SRCS)
#define LC                 0
#define RC                 (NUM_COLUMS - 1)
#define MC                 (NUM_COLUMS/2)

#define PIN_SIZE           8
#define INVERT_SIZE        14

PinMap::PinMap()
{
  MouseInit();

  pinsOrg = gcnew Dictionary<String ^, PinSource ^>;
  pins = gcnew Dictionary<String ^, Pin ^>;

  int C, L, IC;

  C = LC;
  L = UL;
  IC = 0;
  pins["RX(A)"]   = gcnew Pin(C, L++, true,  0);
  pins["TX(A)"]   = gcnew Pin(C, L++, true,  0);
  pins["DTR(A)"]  = gcnew Pin(C, L++, false, IC++);
  pins["DSR(A)"]  = gcnew Pin(C, L++, false, 0);
  pins["DCD(A)"]  = gcnew Pin(C, L++, false, 0);
  pins["RTS(A)"]  = gcnew Pin(C, L++, false, IC++);
  pins["CTS(A)"]  = gcnew Pin(C, L++, false, 0);
  pins["RI(A)"]   = gcnew Pin(C, L++, false, 0);
  pins["OUT1(A)"] = gcnew Pin(C, L++, false, IC++);
  pins["OPEN(A)"] = gcnew Pin(C, L++, false, IC++);

  C = RC;
  L = UL;
  IC = 0;
  pins["RX(B)"]   = gcnew Pin(C, L++, true,  0);
  pins["TX(B)"]   = gcnew Pin(C, L++, true,  0);
  pins["DTR(B)"]  = gcnew Pin(C, L++, false, IC++);
  pins["DSR(B)"]  = gcnew Pin(C, L++, false, 0);
  pins["DCD(B)"]  = gcnew Pin(C, L++, false, 0);
  pins["RTS(B)"]  = gcnew Pin(C, L++, false, IC++);
  pins["CTS(B)"]  = gcnew Pin(C, L++, false, 0);
  pins["RI(B)"]   = gcnew Pin(C, L++, false, 0);
  pins["OUT1(B)"] = gcnew Pin(C, L++, false, IC++);
  pins["OPEN(B)"] = gcnew Pin(C, L++, false, IC++);

  pins["ON"]      = gcnew Pin(MC, DL, false, 0);

  pins["RX(A)"]  -> Link(gcnew PinSource("TX(B)",  false));
  pins["RX(B)"]  -> Link(gcnew PinSource("TX(A)",  false));

  Init(nullptr);
}

void PinMap::Link(String ^pinName, String ^source, bool invert)
{
  pinsOrg[pinName] = gcnew PinSource(source, invert);
  pins[pinName] -> Link(pinsOrg[pinName]);
}

void PinMap::Init(PortPair ^pair)
{
  MouseInit();

  array<String ^> ^dstPins = {"DSR", "DCD", "CTS", "RI"};

  for each (String ^dst in dstPins) {
    for (int i = 0 ; i < 2 ; i++) {
      if (pair == nullptr ||
          pair[i] == nullptr ||
          !pair[i]->ContainsKey(dst->ToLower()))
      {
        Link(String::Format("{0}{1}", dst, (i == 0) ? "(A)" : "(B)"), "", false);
        continue;
      }

      String ^src = pair[i][dst->ToLower()];

      bool invert = false;

      if (src->StartsWith("!")) {
        invert = true;
        src = src->Substring(1);
      }

      String ^srcSuffix = "";

      if (src->StartsWith("L")) {
        srcSuffix = (i == 0) ? "(A)" : "(B)";
        src = src->Substring(1);
      }
      else
      if (src->StartsWith("R")) {
        srcSuffix = (i == 1) ? "(A)" : "(B)";
        src = src->Substring(1);
      }

      Link(String::Format("{0}{1}", dst, (i == 0) ? "(A)" : "(B)"),
           String::Format("{0}{1}", src, srcSuffix), invert);
    }
  }
}

void PinMap::GetChanges(PortPair ^pair)
{
  for each (KeyValuePair<String ^, Pin ^> kvp in pins) {
    Pin ^pin = kvp.Value;

    if (!pin->isFixed && !pin->IsSource() &&
        (!pinsOrg->ContainsKey(kvp.Key) || !pin->source->IsEqual(pinsOrg[kvp.Key])))
    {
      array<Char> ^separator = {'('};

      array<String ^> ^fields;
      
      fields = kvp.Key->Split(separator);

      if (fields->Length != 2)
        continue;

      String ^dst = fields[0];
      int iDst;

      if (fields[1]->StartsWith("A"))
        iDst = 0;
      else
      if (fields[1]->StartsWith("B"))
        iDst = 1;
      else
        continue;

      String ^src = pin->source->invert ? "!" : "";

      fields = pin->source->source->Split(separator);

      if (fields->Length > 1) {
        if (fields[1]->StartsWith("A")) {
          src += (iDst == 0) ? "l" : "r";
        }
        else
        if (fields[1]->StartsWith("B")) {
          src += (iDst == 1) ? "l" : "r";
        }
      }

      src += fields[0];

      pair[iDst][dst->ToLower()] = src->ToUpper();
    }
  }
}

void PinMap::Paint(PaintEventArgs ^e, Control ^control)
{
  Pen ^pOrg = gcnew Pen(Color::Black, 1);
  pOrg->EndCap = Drawing2D::LineCap::ArrowAnchor;

  Pen ^pNew = gcnew Pen(Color::Blue, 1);
  pNew->EndCap = Drawing2D::LineCap::ArrowAnchor;

  Pen ^p = pOrg;

  SolidBrush ^b = gcnew SolidBrush(Color::Black);
  SolidBrush ^bS = gcnew SolidBrush(Color::Red);
  SolidBrush ^bD = gcnew SolidBrush(Color::Green);

  int width = control->ClientSize.Width;
  int height = control->ClientSize.Height;

  int stepC = width/NUM_COLUMS;
  int stepL = height/NUM_LINES;
  int xCorrection = (width - NUM_COLUMS*stepC)/2;
  int yCorrection = (height - NUM_LINES*stepL)/2;

  for each (KeyValuePair<String ^, Pin ^> kvp in pins) {
    Pin ^pin = kvp.Value;
    int x, y;

    x = xCorrection + stepC/2 + pin->C * stepC;
    y = yCorrection + stepL/2 + pin->L * stepL;

    int pinSize = PIN_SIZE;

    if (!pin->isFixed &&
        (!pins->ContainsKey(selected) || pins[selected]->IsSource() != pin->IsSource()) &&
        me.X > x - stepC && me.X < x + stepC &&
        me.Y > y - stepL/2 && me.Y < y + stepL/2)
    {
      if (down) {
        mb.X = x;
        mb.Y = y;
        selected = kvp.Key;
      } else {
        pinSize += 4;
      }

      if (doubleClick)
        pin->Invert();

      if (up && pins->ContainsKey(selected)) {
        String ^src, ^dst;

        if (pin->IsSource()) {
          src = kvp.Key;
          dst = selected;
        } else {
          src = selected;
          dst = kvp.Key;
        }

        if (pinsOrg[dst]->source == src)
          pins[dst]->Link(pinsOrg[dst]);
        else
          pins[dst]->Link(gcnew PinSource(src, false));
      }
    }

    Brush ^bE = b;

    if (!pin->isFixed) {
      if (pin->IsSource())
        bE = bS;
      else
        bE = bD;
    }

    e->Graphics->FillEllipse(bE, x - pinSize/2, y - pinSize/2, pinSize, pinSize);

    if (pin->C == LC) {
      if (pin->IsSource())
        e->Graphics->DrawLine(p, 0, y, x - PIN_SIZE/2, y);
      else
        e->Graphics->DrawLine(p, x - PIN_SIZE/2, y, 0, y);
    }
    else
    if (pin->C == RC) {
      if (pin->IsSource())
        e->Graphics->DrawLine(p, width, y, x + PIN_SIZE/2, y);
      else
        e->Graphics->DrawLine(p, x + PIN_SIZE/2, y, width, y);
    }
    else
    if (pin->L == UL) {
      if (pin->IsSource())
        e->Graphics->DrawLine(p, x, 0, x, y - PIN_SIZE/2);
      else
        e->Graphics->DrawLine(p, x, y - PIN_SIZE/2, x, 0);
    }
    else
    if (pin->L == DL) {
      if (pin->IsSource())
        e->Graphics->DrawLine(p, x, height, x, y + PIN_SIZE/2);
      else
        e->Graphics->DrawLine(p, x, y + PIN_SIZE/2, x, height);
    }
  }

  for each (KeyValuePair<String ^, Pin ^> kvp in pins) {
    Pin ^pin = kvp.Value;

    if (!pin->source)
      continue;

    String ^source = pin->source->source;

    if (pins->ContainsKey(source)) {
      Pin ^pinS = pins[source];

      int xS = xCorrection + stepC/2 + pinS->C * stepC;
      int yS = yCorrection + stepL/2 + pinS->L * stepL;

      array<Point> ^line = gcnew array<Point>(4);

      if (pinS->C == LC) {
        line[0].X = xS + PIN_SIZE/2;
        line[0].Y = yS;
      }
      else
      if (pinS->C == RC) {
        line[0].X = xS - PIN_SIZE/2;
        line[0].Y = yS;
      }
      else
      if (pinS->L == UL) {
        line[0].X = xS;
        line[0].Y = yS + PIN_SIZE/2;
      }
      else
      if (pinS->L == DL) {
        line[0].X = xS;
        line[0].Y = yS - PIN_SIZE/2;
      }
      else {
        line[0].X = xS;
        line[0].Y = yS;
      }

      int xD = xCorrection + stepC/2 + pin->C * stepC;
      int yD = yCorrection + stepL/2 + pin->L * stepL;

      Pen ^pCur = (pin->isFixed || pin->source->IsEqual(pinsOrg[kvp.Key])) ? pOrg : pNew;

      if (pin->C == LC) {
        line[3].X = xD + PIN_SIZE/2;
        line[3].Y = yD;

        if (pin->source->invert) {
          line[3].X += INVERT_SIZE/2;

          array<Point> ^poly = {
            Point(line[3].X, line[3].Y - INVERT_SIZE/2),
            Point(line[3].X, line[3].Y + INVERT_SIZE/2),
            Point(line[3].X - INVERT_SIZE/2, line[3].Y),
          };

          e->Graphics->DrawPolygon(pCur, poly);
        }
      }
      else
      if (pin->C == RC) {
        line[3].X = xD - PIN_SIZE/2;
        line[3].Y = yD;

        if (pin->source->invert) {
          line[3].X -= INVERT_SIZE/2;

          array<Point> ^poly = {
            Point(line[3].X, line[3].Y - INVERT_SIZE/2),
            Point(line[3].X, line[3].Y + INVERT_SIZE/2),
            Point(line[3].X + INVERT_SIZE/2, line[3].Y),
          };

          Pen ^pCur = (pin->isFixed || pin->source->IsEqual(pinsOrg[kvp.Key])) ? pOrg : pNew;
          e->Graphics->DrawPolygon(pCur, poly);
        }
      }
      else
      if (pin->L == UL) {
        line[3].X = xD;
        line[3].Y = yD + PIN_SIZE/2;

        if (pin->source->invert) {
          line[3].Y += INVERT_SIZE/2;

          array<Point> ^poly = {
            Point(line[3].X - INVERT_SIZE/2, line[3].Y),
            Point(line[3].X + INVERT_SIZE/2, line[3].Y),
            Point(line[3].X, line[3].Y - INVERT_SIZE/2),
          };

          Pen ^pCur = (pin->isFixed || pin->source->IsEqual(pinsOrg[kvp.Key])) ? pOrg : pNew;
          e->Graphics->DrawPolygon(pCur, poly);
        }
      }
      else
      if (pin->L == DL) {
        line[3].X = xD;
        line[3].Y = yD - PIN_SIZE/2;

        if (pin->source->invert) {
          line[3].Y -= INVERT_SIZE/2;

          array<Point> ^poly = {
            Point(line[3].X - INVERT_SIZE/2, line[3].Y),
            Point(line[3].X + INVERT_SIZE/2, line[3].Y),
            Point(line[3].X, line[3].Y + INVERT_SIZE/2),
          };

          Pen ^pCur = (pin->isFixed || pin->source->IsEqual(pinsOrg[kvp.Key])) ? pOrg : pNew;
          e->Graphics->DrawPolygon(pCur, poly);
        }
      }
      else {
        line[3].X = xD;
        line[3].Y = yD;
      }

      line[1].Y = line[0].Y;
      line[2].Y = line[3].Y;

      int diffX = (pin->L - pinS->L) * stepC;
      if (diffX < 0)
        diffX = -diffX;

      if (pinS->C != pin->C) {
        if (pinS->C == LC) {
          line[1].X = xCorrection + (NUM_COLUMS*stepC)/2 - stepC/2;
          line[2].X = line[1].X + diffX;
        }
        else
        if (pinS->C == RC) {
          line[1].X = xCorrection + (NUM_COLUMS*stepC)/2 + stepC/2;
          line[2].X = line[1].X - diffX;
        }
        else {
          line[1].X = line[0].X;
          line[1].Y = line[2].Y;
          line[2].X = line[3].X;
        }
      } else {
        if (pinS->C == LC) {
          line[1].X = line[0].X + (stepC + pinS->IC * stepC);
          line[2].X = line[1].X;
        }
        else
        if (pinS->C == RC) {
          line[1].X = xS - (stepC + pinS->IC * stepC);
          line[2].X = line[1].X;
        }
        else {
          line[1].X = line[0].X;
          line[2].X = line[3].X;
        }
      }

      e->Graphics->DrawLines(pCur, line);
    }
  }

  down = false;
  doubleClick = false;

  if (up) {
    selected = "";
    up = false;
  }

  if (pins->ContainsKey(selected)) {
    p->EndCap = Drawing2D::LineCap::NoAnchor;
    e->Graphics->DrawLine(p, mb.X, mb.Y, me.X, me.Y);
  }
}

void PinMap::MouseInit()
{
  mb = me = Point(0, 0);
  selected = "";
  down = up = doubleClick = false;
}

void PinMap::MouseDown(MouseEventArgs ^e)
{
  down = true;
  me = Point(e->X, e->Y);
}

void PinMap::MouseMove(MouseEventArgs ^e)
{
  me = Point(e->X, e->Y);
}

void PinMap::MouseUp(MouseEventArgs ^e)
{
  up = true;
  me = Point(e->X, e->Y);
}

void PinMap::MouseDoubleClick(MouseEventArgs ^e)
{
  MouseInit();
  doubleClick = true;
  me = Point(e->X, e->Y);
}
