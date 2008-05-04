/*
 * $Id$
 *
 * Copyright (c) 2007-2008 Vyacheslav Frolov
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
 * Revision 1.4  2008/05/04 09:56:47  vfrolov
 * Implemented HiddenMode option
 *
 * Revision 1.3  2008/04/08 06:52:12  vfrolov
 * Added pin OUT2
 *
 * Revision 1.2  2007/11/20 12:29:40  vfrolov
 * Fixed appearance for Vista
 *
 * Revision 1.1  2007/10/31 10:16:55  vfrolov
 * Initial revision
 *
 */

#pragma once

namespace SetupApp {

    using namespace System::Collections::Generic;
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
    public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}

    private: System::Windows::Forms::TextBox^  PortNameB;
    private: System::Windows::Forms::TextBox^  PortNameA;
    private: System::Windows::Forms::PictureBox^  picturePinMap;
    private: System::Windows::Forms::ToolTip^  toolTip1;
    private: System::ComponentModel::IContainer^  components;
    private: System::Windows::Forms::Button^  buttonRemovePair;
    private: System::Windows::Forms::Button^  buttonAddPair;
    private: System::Windows::Forms::Button^  buttonApply;
    private: System::Windows::Forms::Button^  buttonReset;
    private: System::Windows::Forms::CheckBox^  EmuBrB;
    private: System::Windows::Forms::CheckBox^  EmuOverrunB;
    private: System::Windows::Forms::CheckBox^  PlugInModeB;
    private: System::Windows::Forms::CheckBox^  ExclusiveModeB;
    private: System::Windows::Forms::CheckBox^  ExclusiveModeA;
    private: System::Windows::Forms::CheckBox^  PlugInModeA;
    private: System::Windows::Forms::CheckBox^  EmuOverrunA;
    private: System::Windows::Forms::CheckBox^  EmuBrA;
    private: System::Windows::Forms::CheckBox^  HiddenModeA;
    private: System::Windows::Forms::CheckBox^  HiddenModeB;
    private: System::Windows::Forms::Label^  pinNameON;
    private: System::Windows::Forms::Label^  pinNameA_RX;
    private: System::Windows::Forms::Label^  pinNameA_DTR;
    private: System::Windows::Forms::Label^  pinNameA_TX;
    private: System::Windows::Forms::Label^  pinNameA_DSR;
    private: System::Windows::Forms::Label^  pinNameA_DCD;
    private: System::Windows::Forms::Label^  pinNameA_RTS;
    private: System::Windows::Forms::Label^  pinNameA_CTS;
    private: System::Windows::Forms::Label^  pinNameA_RI;
    private: System::Windows::Forms::Label^  pinNameA_OUT1;
    private: System::Windows::Forms::Label^  pinNameA_OUT2;
    private: System::Windows::Forms::Label^  pinNameA_OPEN;
    private: System::Windows::Forms::Label^  pinNameB_OPEN;
    private: System::Windows::Forms::Label^  pinNameB_OUT1;
    private: System::Windows::Forms::Label^  pinNameB_OUT2;
    private: System::Windows::Forms::Label^  pinNameB_RI;
    private: System::Windows::Forms::Label^  pinNameB_CTS;
    private: System::Windows::Forms::Label^  pinNameB_RTS;
    private: System::Windows::Forms::Label^  pinNameB_DCD;
    private: System::Windows::Forms::Label^  pinNameB_DSR;
    private: System::Windows::Forms::Label^  pinNameB_TX;
    private: System::Windows::Forms::Label^  pinNameB_DTR;
    private: System::Windows::Forms::Label^  pinNameB_RX;
    private: System::Windows::Forms::TreeView^  pairList;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
          this->components = (gcnew System::ComponentModel::Container());
          System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
          this->PortNameB = (gcnew System::Windows::Forms::TextBox());
          this->PortNameA = (gcnew System::Windows::Forms::TextBox());
          this->pinNameON = (gcnew System::Windows::Forms::Label());
          this->picturePinMap = (gcnew System::Windows::Forms::PictureBox());
          this->toolTip1 = (gcnew System::Windows::Forms::ToolTip(this->components));
          this->EmuBrB = (gcnew System::Windows::Forms::CheckBox());
          this->EmuOverrunB = (gcnew System::Windows::Forms::CheckBox());
          this->PlugInModeB = (gcnew System::Windows::Forms::CheckBox());
          this->ExclusiveModeB = (gcnew System::Windows::Forms::CheckBox());
          this->ExclusiveModeA = (gcnew System::Windows::Forms::CheckBox());
          this->PlugInModeA = (gcnew System::Windows::Forms::CheckBox());
          this->EmuOverrunA = (gcnew System::Windows::Forms::CheckBox());
          this->EmuBrA = (gcnew System::Windows::Forms::CheckBox());
          this->HiddenModeA = (gcnew System::Windows::Forms::CheckBox());
          this->HiddenModeB = (gcnew System::Windows::Forms::CheckBox());
          this->pinNameA_RX = (gcnew System::Windows::Forms::Label());
          this->pinNameA_DTR = (gcnew System::Windows::Forms::Label());
          this->pinNameA_TX = (gcnew System::Windows::Forms::Label());
          this->pinNameA_DSR = (gcnew System::Windows::Forms::Label());
          this->pinNameA_DCD = (gcnew System::Windows::Forms::Label());
          this->pinNameA_RTS = (gcnew System::Windows::Forms::Label());
          this->pinNameA_CTS = (gcnew System::Windows::Forms::Label());
          this->pinNameA_RI = (gcnew System::Windows::Forms::Label());
          this->pinNameA_OUT1 = (gcnew System::Windows::Forms::Label());
          this->pinNameA_OUT2 = (gcnew System::Windows::Forms::Label());
          this->pinNameA_OPEN = (gcnew System::Windows::Forms::Label());
          this->pinNameB_OPEN = (gcnew System::Windows::Forms::Label());
          this->pinNameB_OUT1 = (gcnew System::Windows::Forms::Label());
          this->pinNameB_OUT2 = (gcnew System::Windows::Forms::Label());
          this->pinNameB_RI = (gcnew System::Windows::Forms::Label());
          this->pinNameB_CTS = (gcnew System::Windows::Forms::Label());
          this->pinNameB_RTS = (gcnew System::Windows::Forms::Label());
          this->pinNameB_DCD = (gcnew System::Windows::Forms::Label());
          this->pinNameB_DSR = (gcnew System::Windows::Forms::Label());
          this->pinNameB_TX = (gcnew System::Windows::Forms::Label());
          this->pinNameB_DTR = (gcnew System::Windows::Forms::Label());
          this->pinNameB_RX = (gcnew System::Windows::Forms::Label());
          this->pairList = (gcnew System::Windows::Forms::TreeView());
          this->buttonRemovePair = (gcnew System::Windows::Forms::Button());
          this->buttonAddPair = (gcnew System::Windows::Forms::Button());
          this->buttonApply = (gcnew System::Windows::Forms::Button());
          this->buttonReset = (gcnew System::Windows::Forms::Button());
          (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->picturePinMap))->BeginInit();
          this->SuspendLayout();
          // 
          // PortNameB
          // 
          resources->ApplyResources(this->PortNameB, L"PortNameB");
          this->PortNameB->Name = L"PortNameB";
          this->toolTip1->SetToolTip(this->PortNameB, resources->GetString(L"PortNameB.ToolTip"));
          this->PortNameB->TextChanged += gcnew System::EventHandler(this, &Form1::PortNameB_Changed);
          // 
          // PortNameA
          // 
          resources->ApplyResources(this->PortNameA, L"PortNameA");
          this->PortNameA->Name = L"PortNameA";
          this->toolTip1->SetToolTip(this->PortNameA, resources->GetString(L"PortNameA.ToolTip"));
          this->PortNameA->TextChanged += gcnew System::EventHandler(this, &Form1::PortNameA_Changed);
          // 
          // pinNameON
          // 
          resources->ApplyResources(this->pinNameON, L"pinNameON");
          this->pinNameON->Name = L"pinNameON";
          this->toolTip1->SetToolTip(this->pinNameON, resources->GetString(L"pinNameON.ToolTip"));
          // 
          // picturePinMap
          // 
          this->picturePinMap->BackColor = System::Drawing::SystemColors::Control;
          this->picturePinMap->Cursor = System::Windows::Forms::Cursors::Hand;
          resources->ApplyResources(this->picturePinMap, L"picturePinMap");
          this->picturePinMap->Name = L"picturePinMap";
          this->picturePinMap->TabStop = false;
          this->toolTip1->SetToolTip(this->picturePinMap, resources->GetString(L"picturePinMap.ToolTip"));
          this->picturePinMap->MouseLeave += gcnew System::EventHandler(this, &Form1::picturePinMap_MouseLeave);
          this->picturePinMap->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::picturePinMap_MouseDown);
          this->picturePinMap->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::picturePinMap_MouseMove);
          this->picturePinMap->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &Form1::picturePinMap_Paint);
          this->picturePinMap->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::picturePinMap_MouseDoubleClick);
          this->picturePinMap->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &Form1::picturePinMap_MouseUp);
          // 
          // EmuBrB
          // 
          resources->ApplyResources(this->EmuBrB, L"EmuBrB");
          this->EmuBrB->Name = L"EmuBrB";
          this->toolTip1->SetToolTip(this->EmuBrB, resources->GetString(L"EmuBrB.ToolTip"));
          this->EmuBrB->UseVisualStyleBackColor = true;
          this->EmuBrB->CheckedChanged += gcnew System::EventHandler(this, &Form1::EmuBrB_Changed);
          // 
          // EmuOverrunB
          // 
          resources->ApplyResources(this->EmuOverrunB, L"EmuOverrunB");
          this->EmuOverrunB->Name = L"EmuOverrunB";
          this->toolTip1->SetToolTip(this->EmuOverrunB, resources->GetString(L"EmuOverrunB.ToolTip"));
          this->EmuOverrunB->UseVisualStyleBackColor = true;
          this->EmuOverrunB->CheckedChanged += gcnew System::EventHandler(this, &Form1::EmuOverrunB_Changed);
          // 
          // PlugInModeB
          // 
          resources->ApplyResources(this->PlugInModeB, L"PlugInModeB");
          this->PlugInModeB->Name = L"PlugInModeB";
          this->toolTip1->SetToolTip(this->PlugInModeB, resources->GetString(L"PlugInModeB.ToolTip"));
          this->PlugInModeB->UseVisualStyleBackColor = true;
          this->PlugInModeB->CheckedChanged += gcnew System::EventHandler(this, &Form1::PlugInModeB_Changed);
          // 
          // ExclusiveModeB
          // 
          resources->ApplyResources(this->ExclusiveModeB, L"ExclusiveModeB");
          this->ExclusiveModeB->Name = L"ExclusiveModeB";
          this->toolTip1->SetToolTip(this->ExclusiveModeB, resources->GetString(L"ExclusiveModeB.ToolTip"));
          this->ExclusiveModeB->UseVisualStyleBackColor = true;
          this->ExclusiveModeB->CheckedChanged += gcnew System::EventHandler(this, &Form1::ExclusiveModeB_Changed);
          // 
          // ExclusiveModeA
          // 
          resources->ApplyResources(this->ExclusiveModeA, L"ExclusiveModeA");
          this->ExclusiveModeA->Name = L"ExclusiveModeA";
          this->toolTip1->SetToolTip(this->ExclusiveModeA, resources->GetString(L"ExclusiveModeA.ToolTip"));
          this->ExclusiveModeA->UseVisualStyleBackColor = true;
          this->ExclusiveModeA->CheckedChanged += gcnew System::EventHandler(this, &Form1::ExclusiveModeA_Changed);
          // 
          // PlugInModeA
          // 
          resources->ApplyResources(this->PlugInModeA, L"PlugInModeA");
          this->PlugInModeA->Name = L"PlugInModeA";
          this->toolTip1->SetToolTip(this->PlugInModeA, resources->GetString(L"PlugInModeA.ToolTip"));
          this->PlugInModeA->UseVisualStyleBackColor = true;
          this->PlugInModeA->CheckedChanged += gcnew System::EventHandler(this, &Form1::PlugInModeA_Changed);
          // 
          // EmuOverrunA
          // 
          resources->ApplyResources(this->EmuOverrunA, L"EmuOverrunA");
          this->EmuOverrunA->Name = L"EmuOverrunA";
          this->toolTip1->SetToolTip(this->EmuOverrunA, resources->GetString(L"EmuOverrunA.ToolTip"));
          this->EmuOverrunA->UseVisualStyleBackColor = true;
          this->EmuOverrunA->CheckedChanged += gcnew System::EventHandler(this, &Form1::EmuOverrunA_Changed);
          // 
          // EmuBrA
          // 
          this->EmuBrA->ForeColor = System::Drawing::SystemColors::ControlText;
          resources->ApplyResources(this->EmuBrA, L"EmuBrA");
          this->EmuBrA->Name = L"EmuBrA";
          this->toolTip1->SetToolTip(this->EmuBrA, resources->GetString(L"EmuBrA.ToolTip"));
          this->EmuBrA->UseVisualStyleBackColor = true;
          this->EmuBrA->CheckedChanged += gcnew System::EventHandler(this, &Form1::EmuBrA_Changed);
          // 
          // HiddenModeA
          // 
          resources->ApplyResources(this->HiddenModeA, L"HiddenModeA");
          this->HiddenModeA->Name = L"HiddenModeA";
          this->toolTip1->SetToolTip(this->HiddenModeA, resources->GetString(L"HiddenModeA.ToolTip"));
          this->HiddenModeA->UseVisualStyleBackColor = true;
          this->HiddenModeA->CheckedChanged += gcnew System::EventHandler(this, &Form1::HiddenModeA_Changed);
          // 
          // HiddenModeB
          // 
          resources->ApplyResources(this->HiddenModeB, L"HiddenModeB");
          this->HiddenModeB->Name = L"HiddenModeB";
          this->toolTip1->SetToolTip(this->HiddenModeB, resources->GetString(L"HiddenModeB.ToolTip"));
          this->HiddenModeB->UseVisualStyleBackColor = true;
          this->HiddenModeB->CheckedChanged += gcnew System::EventHandler(this, &Form1::HiddenModeB_Changed);
          // 
          // pinNameA_RX
          // 
          resources->ApplyResources(this->pinNameA_RX, L"pinNameA_RX");
          this->pinNameA_RX->Name = L"pinNameA_RX";
          this->toolTip1->SetToolTip(this->pinNameA_RX, resources->GetString(L"pinNameA_RX.ToolTip"));
          // 
          // pinNameA_DTR
          // 
          resources->ApplyResources(this->pinNameA_DTR, L"pinNameA_DTR");
          this->pinNameA_DTR->Name = L"pinNameA_DTR";
          this->toolTip1->SetToolTip(this->pinNameA_DTR, resources->GetString(L"pinNameA_DTR.ToolTip"));
          // 
          // pinNameA_TX
          // 
          resources->ApplyResources(this->pinNameA_TX, L"pinNameA_TX");
          this->pinNameA_TX->Name = L"pinNameA_TX";
          this->toolTip1->SetToolTip(this->pinNameA_TX, resources->GetString(L"pinNameA_TX.ToolTip"));
          // 
          // pinNameA_DSR
          // 
          resources->ApplyResources(this->pinNameA_DSR, L"pinNameA_DSR");
          this->pinNameA_DSR->Name = L"pinNameA_DSR";
          this->toolTip1->SetToolTip(this->pinNameA_DSR, resources->GetString(L"pinNameA_DSR.ToolTip"));
          // 
          // pinNameA_DCD
          // 
          resources->ApplyResources(this->pinNameA_DCD, L"pinNameA_DCD");
          this->pinNameA_DCD->Name = L"pinNameA_DCD";
          this->toolTip1->SetToolTip(this->pinNameA_DCD, resources->GetString(L"pinNameA_DCD.ToolTip"));
          // 
          // pinNameA_RTS
          // 
          resources->ApplyResources(this->pinNameA_RTS, L"pinNameA_RTS");
          this->pinNameA_RTS->Name = L"pinNameA_RTS";
          this->toolTip1->SetToolTip(this->pinNameA_RTS, resources->GetString(L"pinNameA_RTS.ToolTip"));
          // 
          // pinNameA_CTS
          // 
          resources->ApplyResources(this->pinNameA_CTS, L"pinNameA_CTS");
          this->pinNameA_CTS->Name = L"pinNameA_CTS";
          this->toolTip1->SetToolTip(this->pinNameA_CTS, resources->GetString(L"pinNameA_CTS.ToolTip"));
          // 
          // pinNameA_RI
          // 
          resources->ApplyResources(this->pinNameA_RI, L"pinNameA_RI");
          this->pinNameA_RI->Name = L"pinNameA_RI";
          this->toolTip1->SetToolTip(this->pinNameA_RI, resources->GetString(L"pinNameA_RI.ToolTip"));
          // 
          // pinNameA_OUT1
          // 
          resources->ApplyResources(this->pinNameA_OUT1, L"pinNameA_OUT1");
          this->pinNameA_OUT1->Name = L"pinNameA_OUT1";
          this->toolTip1->SetToolTip(this->pinNameA_OUT1, resources->GetString(L"pinNameA_OUT1.ToolTip"));
          // 
          // pinNameA_OUT2
          // 
          resources->ApplyResources(this->pinNameA_OUT2, L"pinNameA_OUT2");
          this->pinNameA_OUT2->Name = L"pinNameA_OUT2";
          this->toolTip1->SetToolTip(this->pinNameA_OUT2, resources->GetString(L"pinNameA_OUT2.ToolTip"));
          // 
          // pinNameA_OPEN
          // 
          resources->ApplyResources(this->pinNameA_OPEN, L"pinNameA_OPEN");
          this->pinNameA_OPEN->Name = L"pinNameA_OPEN";
          this->toolTip1->SetToolTip(this->pinNameA_OPEN, resources->GetString(L"pinNameA_OPEN.ToolTip"));
          // 
          // pinNameB_OPEN
          // 
          resources->ApplyResources(this->pinNameB_OPEN, L"pinNameB_OPEN");
          this->pinNameB_OPEN->Name = L"pinNameB_OPEN";
          this->toolTip1->SetToolTip(this->pinNameB_OPEN, resources->GetString(L"pinNameB_OPEN.ToolTip"));
          // 
          // pinNameB_OUT1
          // 
          resources->ApplyResources(this->pinNameB_OUT1, L"pinNameB_OUT1");
          this->pinNameB_OUT1->Name = L"pinNameB_OUT1";
          this->toolTip1->SetToolTip(this->pinNameB_OUT1, resources->GetString(L"pinNameB_OUT1.ToolTip"));
          // 
          // pinNameB_OUT2
          // 
          resources->ApplyResources(this->pinNameB_OUT2, L"pinNameB_OUT2");
          this->pinNameB_OUT2->Name = L"pinNameB_OUT2";
          this->toolTip1->SetToolTip(this->pinNameB_OUT2, resources->GetString(L"pinNameB_OUT2.ToolTip"));
          // 
          // pinNameB_RI
          // 
          resources->ApplyResources(this->pinNameB_RI, L"pinNameB_RI");
          this->pinNameB_RI->Name = L"pinNameB_RI";
          this->toolTip1->SetToolTip(this->pinNameB_RI, resources->GetString(L"pinNameB_RI.ToolTip"));
          // 
          // pinNameB_CTS
          // 
          resources->ApplyResources(this->pinNameB_CTS, L"pinNameB_CTS");
          this->pinNameB_CTS->Name = L"pinNameB_CTS";
          this->toolTip1->SetToolTip(this->pinNameB_CTS, resources->GetString(L"pinNameB_CTS.ToolTip"));
          // 
          // pinNameB_RTS
          // 
          resources->ApplyResources(this->pinNameB_RTS, L"pinNameB_RTS");
          this->pinNameB_RTS->Name = L"pinNameB_RTS";
          this->toolTip1->SetToolTip(this->pinNameB_RTS, resources->GetString(L"pinNameB_RTS.ToolTip"));
          // 
          // pinNameB_DCD
          // 
          resources->ApplyResources(this->pinNameB_DCD, L"pinNameB_DCD");
          this->pinNameB_DCD->Name = L"pinNameB_DCD";
          this->toolTip1->SetToolTip(this->pinNameB_DCD, resources->GetString(L"pinNameB_DCD.ToolTip"));
          // 
          // pinNameB_DSR
          // 
          resources->ApplyResources(this->pinNameB_DSR, L"pinNameB_DSR");
          this->pinNameB_DSR->Name = L"pinNameB_DSR";
          this->toolTip1->SetToolTip(this->pinNameB_DSR, resources->GetString(L"pinNameB_DSR.ToolTip"));
          // 
          // pinNameB_TX
          // 
          resources->ApplyResources(this->pinNameB_TX, L"pinNameB_TX");
          this->pinNameB_TX->Name = L"pinNameB_TX";
          this->toolTip1->SetToolTip(this->pinNameB_TX, resources->GetString(L"pinNameB_TX.ToolTip"));
          // 
          // pinNameB_DTR
          // 
          resources->ApplyResources(this->pinNameB_DTR, L"pinNameB_DTR");
          this->pinNameB_DTR->Name = L"pinNameB_DTR";
          this->toolTip1->SetToolTip(this->pinNameB_DTR, resources->GetString(L"pinNameB_DTR.ToolTip"));
          // 
          // pinNameB_RX
          // 
          resources->ApplyResources(this->pinNameB_RX, L"pinNameB_RX");
          this->pinNameB_RX->Name = L"pinNameB_RX";
          this->toolTip1->SetToolTip(this->pinNameB_RX, resources->GetString(L"pinNameB_RX.ToolTip"));
          // 
          // pairList
          // 
          this->pairList->HideSelection = false;
          resources->ApplyResources(this->pairList, L"pairList");
          this->pairList->Name = L"pairList";
          this->pairList->AfterSelect += gcnew System::Windows::Forms::TreeViewEventHandler(this, &Form1::pairsList_AfterSelect);
          this->pairList->BeforeSelect += gcnew System::Windows::Forms::TreeViewCancelEventHandler(this, &Form1::pairsList_BeforeSelect);
          // 
          // buttonRemovePair
          // 
          resources->ApplyResources(this->buttonRemovePair, L"buttonRemovePair");
          this->buttonRemovePair->Name = L"buttonRemovePair";
          this->buttonRemovePair->UseVisualStyleBackColor = true;
          this->buttonRemovePair->Click += gcnew System::EventHandler(this, &Form1::buttonRemovePair_Click);
          // 
          // buttonAddPair
          // 
          resources->ApplyResources(this->buttonAddPair, L"buttonAddPair");
          this->buttonAddPair->Name = L"buttonAddPair";
          this->buttonAddPair->UseVisualStyleBackColor = true;
          this->buttonAddPair->Click += gcnew System::EventHandler(this, &Form1::buttonAddPair_Click);
          // 
          // buttonApply
          // 
          resources->ApplyResources(this->buttonApply, L"buttonApply");
          this->buttonApply->Name = L"buttonApply";
          this->buttonApply->UseVisualStyleBackColor = true;
          this->buttonApply->Click += gcnew System::EventHandler(this, &Form1::buttonApply_Click);
          // 
          // buttonReset
          // 
          resources->ApplyResources(this->buttonReset, L"buttonReset");
          this->buttonReset->Name = L"buttonReset";
          this->buttonReset->UseVisualStyleBackColor = true;
          this->buttonReset->Click += gcnew System::EventHandler(this, &Form1::buttonReset_Click);
          // 
          // Form1
          // 
          resources->ApplyResources(this, L"$this");
          this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
          this->Controls->Add(this->HiddenModeA);
          this->Controls->Add(this->HiddenModeB);
          this->Controls->Add(this->pinNameB_OPEN);
          this->Controls->Add(this->pinNameB_OUT1);
          this->Controls->Add(this->pinNameB_OUT2);
          this->Controls->Add(this->pinNameB_RI);
          this->Controls->Add(this->pinNameB_CTS);
          this->Controls->Add(this->pinNameB_RTS);
          this->Controls->Add(this->pinNameB_DCD);
          this->Controls->Add(this->pinNameB_DSR);
          this->Controls->Add(this->pinNameB_TX);
          this->Controls->Add(this->pinNameB_DTR);
          this->Controls->Add(this->pinNameB_RX);
          this->Controls->Add(this->pinNameA_OPEN);
          this->Controls->Add(this->pinNameA_OUT1);
          this->Controls->Add(this->pinNameA_OUT2);
          this->Controls->Add(this->pinNameA_RI);
          this->Controls->Add(this->pinNameA_CTS);
          this->Controls->Add(this->pinNameA_RTS);
          this->Controls->Add(this->pinNameA_DCD);
          this->Controls->Add(this->pinNameA_DSR);
          this->Controls->Add(this->pinNameA_TX);
          this->Controls->Add(this->pinNameA_DTR);
          this->Controls->Add(this->pinNameA_RX);
          this->Controls->Add(this->ExclusiveModeA);
          this->Controls->Add(this->PlugInModeA);
          this->Controls->Add(this->EmuOverrunA);
          this->Controls->Add(this->EmuBrA);
          this->Controls->Add(this->ExclusiveModeB);
          this->Controls->Add(this->PlugInModeB);
          this->Controls->Add(this->EmuOverrunB);
          this->Controls->Add(this->EmuBrB);
          this->Controls->Add(this->buttonReset);
          this->Controls->Add(this->buttonApply);
          this->Controls->Add(this->buttonAddPair);
          this->Controls->Add(this->buttonRemovePair);
          this->Controls->Add(this->pairList);
          this->Controls->Add(this->PortNameB);
          this->Controls->Add(this->PortNameA);
          this->Controls->Add(this->pinNameON);
          this->Controls->Add(this->picturePinMap);
          this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
          this->MaximizeBox = false;
          this->Name = L"Form1";
          this->Load += gcnew System::EventHandler(this, &Form1::this_Load);
          (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->picturePinMap))->EndInit();
          this->ResumeLayout(false);
          this->PerformLayout();

        }
#pragma endregion

    private:

        String ^GetControlValue(CheckBox ^control) {
          return control->Checked ? "YES" : "NO";
        }

        String ^GetControlValue(TextBox ^control) {
          return control->Text;
        }

        Void SetControlValue(CheckBox ^control, String ^value) {
          control->Checked = (value->ToUpper() == "YES") ? true : false;
        }

        Void SetControlValue(TextBox ^control, String ^value) {
          control->Text = value;
        }

        /////////////////////////////////////////////////////////////////////
        #define DeclareControlPair(controlClass, control) \
          array<controlClass ^> ^control##s; \
          Void control##_Init() { \
            control##s = gcnew array<controlClass ^>{control##A, control##B}; \
          } \
          Void control##A_Changed(Object ^/*sender*/, EventArgs ^/*e*/) { \
            control##_Changed(0); \
          } \
          Void control##B_Changed(Object ^/*sender*/, EventArgs ^/*e*/) { \
            control##_Changed(1); \
          } \
          Void control##_Changed(int i) { \
            try { \
              String ^key = (gcnew String(#control))->ToLower(); \
              String ^value = GetControlValue(control##s[i])->ToUpper(); \
              if (pairs[pairList->SelectedNode->Name][i][key] != value) { \
                control##s[i]->ForeColor = Color::Blue; \
                return; \
              } \
            } \
            catch (Exception^ /*e*/) { \
            } \
            control##s[i]->ForeColor = System::Drawing::SystemColors::ControlText; \
          } \
          Void control##_GetChanges(int i, PortPair ^portChanges) { \
            try { \
              String ^key = (gcnew String(#control))->ToLower(); \
              String ^value = GetControlValue(control##s[i])->ToUpper(); \
              if (pairs[pairList->SelectedNode->Name][i][key] != value) \
                portChanges[i][key] = value; \
            } \
            catch (Exception^ /*e*/) { \
            } \
          } \
          Void control##_Reset(int i) { \
            try { \
              String ^key = (gcnew String(#control))->ToLower(); \
              SetControlValue(control##s[i], pairs[pairList->SelectedNode->Name][i][key]); \
            } \
            catch (Exception^ /*e*/) { \
              SetControlValue(control##s[i], ""); \
            } \
            control##_Changed(i); \
          } \
        /////////////////////////////////////////////////////////////////////

    private:

        DeclareControlPair(TextBox, PortName)
        DeclareControlPair(CheckBox, EmuBr)
        DeclareControlPair(CheckBox, EmuOverrun)
        DeclareControlPair(CheckBox, PlugInMode)
        DeclareControlPair(CheckBox, ExclusiveMode)
        DeclareControlPair(CheckBox, HiddenMode)

        #define ForEachControlPair(func) \
          PortName_##func; \
          EmuBr_##func; \
          EmuOverrun_##func; \
          PlugInMode_##func; \
          ExclusiveMode_##func; \
          HiddenMode_##func; \

    private:

        Void Reset() {
          pairList->BeginUpdate();

          for each (TreeNode ^pair in pairList->Nodes) {
            if (!pairs->ContainsKey(pair->Name) ||
                pairs[pair->Name][0] == nullptr ||
                pairs[pair->Name][1] == nullptr)
            {
              pairList->Nodes->Remove(pair);
            }
          }

          for each (KeyValuePair<String ^, PortPair ^> kvpPair in pairs) {
            if (kvpPair.Value[0] == nullptr || kvpPair.Value[1] == nullptr)
              continue;

            TreeNode ^pair;
            bool pairExpand;
            
            if (pairList->Nodes->ContainsKey(kvpPair.Key)) {
              pair = pairList->Nodes[kvpPair.Key];
              pairExpand = pair->IsExpanded;
            } else {
              pair = pairList->Nodes->Add(kvpPair.Key, String::Format(
                  "Virtual Port Pair {0}", kvpPair.Key));
              pairExpand = true;
            }

            bool portExpand[2];

            for (int i = 0 ; i < 2 ; i++) {
              try {
                portExpand[i] = pair->Nodes[i]->IsExpanded;
              }
              catch (Exception^ /*e*/) {
                portExpand[i] = false;
              }
            }

            pair->Nodes->Clear();

            for (int i = 0 ; i < 2 ; i++) {
              TreeNode ^port;

              try {
                port = pair->Nodes->Add(kvpPair.Value[i]["portname"]);
              }
              catch (Exception^ /*e*/) {
                port = pair->Nodes->Add(String::Format("CNC{0}{1}", (i == 0) ? "A" : "B", kvpPair.Key));
              }

              for each (KeyValuePair<String ^, String ^> kvpPort in kvpPair.Value[i]) {
                port->Nodes->Add(String::Format("{0}={1}", kvpPort.Key, kvpPort.Value));
              }

              if (portExpand[i])
                port->Expand();
            }

            if (pairExpand)
              pair->Expand();
          }

          if (pairList->SelectedNode == nullptr) {
            try {
              pairList->SelectedNode = pairList->Nodes[0];
            }
            catch (Exception^ /*e*/) {
            }
          }

          pairList->EndUpdate();

          try {
            pinMap->Init(pairs[pairList->SelectedNode->Name]);
          }
          catch (Exception^ /*e*/) {
            pinMap->Init(nullptr);
          }

          picturePinMap->Invalidate();

          for (int i = 0 ; i < 2 ; i++) {
            ForEachControlPair(Reset(i))
          }
        }

        PortPair ^GetChanges() {
          PortPair ^portChanges = gcnew PortPair;

          pinMap->GetChanges(portChanges);

          for (int i = 0 ; i < 2 ; i++) {
            ForEachControlPair(GetChanges(i, portChanges))
          }

          return portChanges;
        }

        bool SaveChanges() {
          if (!pairList->SelectedNode)
            return true;

          PortPair ^portChanges = GetChanges();

          if (!portChanges->IsEmpty()) {
            String^ msg = String::Format(
                "The parameters of \"{0}\" were changed.\n"
                "Would you like to apply the changes?",
                pairList->SelectedNode->Text);

            System::Windows::Forms::DialogResult res;
            
            res = MessageBox::Show(this, msg, "Apply", MessageBoxButtons::YesNoCancel);

            if (res == System::Windows::Forms::DialogResult::Cancel) {
              return false;
            }
            else
            if (res == System::Windows::Forms::DialogResult::Yes) {
              pairs->ChangePair(pairList->SelectedNode->Name, portChanges);

              Reset();
            }
          }

          return true;
        }

	private:

        PinMap ^pinMap;
        PortPairs ^pairs;

        Void this_Load(Object ^/*sender*/, EventArgs ^/*e*/) {
          ForEachControlPair(Init())

          pinMap = gcnew PinMap;
          pairs = gcnew PortPairs(this);

          pairs->Init();
          Reset();
        }

    private:

        Void picturePinMap_Paint(Object ^/*sender*/, PaintEventArgs ^e) {
            pinMap->Paint(e, picturePinMap);
        }
 
        Void picturePinMap_MouseDown(Object ^/*sender*/, MouseEventArgs ^e) {
            pinMap->MouseDown(e);
            picturePinMap->Invalidate();
        }

        Void picturePinMap_MouseMove(Object ^/*sender*/, MouseEventArgs ^e) {
            pinMap->MouseMove(e);
            picturePinMap->Invalidate();
        }

        Void picturePinMap_MouseUp(Object ^/*sender*/, MouseEventArgs ^e) {
            pinMap->MouseUp(e);
            picturePinMap->Invalidate();
        }

        Void picturePinMap_MouseDoubleClick(Object ^/*sender*/, MouseEventArgs ^e) {
            pinMap->MouseDoubleClick(e);
            picturePinMap->Invalidate();
        }

        Void picturePinMap_MouseLeave(Object ^/*sender*/, EventArgs ^/*e*/) {
            pinMap->MouseInit();
            picturePinMap->Invalidate();
        }

    private:

        Void pairsList_BeforeSelect(Object ^/*sender*/, TreeViewCancelEventArgs ^e) {
          if (e->Node->Level != 0 || !SaveChanges())
            e->Cancel = true;
        }

        Void pairsList_AfterSelect(Object ^/*sender*/, TreeViewEventArgs ^/*e*/) {
          Reset();
        }

    private:

        Void buttonApply_Click(Object ^/*sender*/, EventArgs ^/*e*/) {
          if (pairList->SelectedNode) {
            pairs->ChangePair(pairList->SelectedNode->Name, GetChanges());
            Reset();
          }
        }

        Void buttonRemovePair_Click(Object ^/*sender*/, EventArgs ^/*e*/) {
          if (pairList->SelectedNode != nullptr) {
            String^ msg = String::Format(
                "Would you like to remove \"{0}\"?",
                pairList->SelectedNode->Text);

            System::Windows::Forms::DialogResult res;
            
            res = MessageBox::Show(this, msg, "", MessageBoxButtons::YesNo);

            if (res == System::Windows::Forms::DialogResult::Yes) {
              pairs->RemovePair(pairList->SelectedNode->Name);
              Reset();
            }
          }
        }

        Void buttonAddPair_Click(Object ^/*sender*/, EventArgs ^/*e*/) {
          if (!SaveChanges())
            return;

          String ^key = pairs->AddPair();

          Reset();

          try {
            pairList->SelectedNode = pairList->Nodes[key];
          }
          catch (Exception^ /*e*/) {
          }
        }

        Void buttonReset_Click(Object ^/*sender*/, EventArgs ^/*e*/) {
          pairs->Init();
          Reset();
        }
    };
}
