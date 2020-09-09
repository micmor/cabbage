/*
  Copyright (C) 2016 Rory Walsh

  Cabbage is free software; you can redistribute it
  and/or modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Cabbage is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

#include "CabbageComboBox.h"
#include "../Audio/Plugins/CabbagePluginEditor.h"

//================================================================================================================
// combobox widget
//================================================================================================================
CabbageComboBox::CabbageComboBox (ValueTree wData, CabbagePluginEditor* _owner)
    : pivotx (CabbageWidgetData::getNumProp (wData, CabbageIdentifierIds::pivotx)),
    pivoty (CabbageWidgetData::getNumProp (wData, CabbageIdentifierIds::pivoty)),
    refresh (0),
    name (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::name)),
    tooltipText (String()),
    workingDir (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::workingdir)),
    rotate (CabbageWidgetData::getNumProp (wData, CabbageIdentifierIds::rotate)),
    owner (_owner),
    widgetData (wData)
{
    widgetData.addListener (this);
    setColour (ComboBox::backgroundColourId, Colour::fromString (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::colour)));
    setColour (ComboBox::textColourId, Colour::fromString (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::fontcolour)));
    setTooltip (tooltipText = CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::popuptext));

    setColour (PopupMenu::ColourIds::backgroundColourId, Colour::fromString (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::fontcolour)).brighter (.8f));
    setColour (PopupMenu::ColourIds::highlightedBackgroundColourId, Colour::fromString (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::colour)));
    setColour (PopupMenu::ColourIds::textColourId, Colour::fromString (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::colour)));
    setColour (PopupMenu::ColourIds::highlightedTextColourId, Colour::fromString (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::colour)).darker());

    setEditableText (false);
    setTextWhenNothingSelected (text);
    setWantsKeyboardFocus (false);
    getProperties().set("isPresetCombo", false);
    initialiseCommonAttributes (this, widgetData);

    if (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::filetype).isNotEmpty())
        CabbageWidgetData::setProperty (widgetData, CabbageIdentifierIds::text, "");

    addItemsToCombobox (widgetData);

    if (CabbageWidgetData::getProperty (widgetData, CabbageIdentifierIds::channeltype) == "string" &&
		!CabbageWidgetData::getStringProp(widgetData, CabbageIdentifierIds::filetype).contains("snaps"))
    {
        isStringCombo = true;

        if (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::filetype).isNotEmpty())
            CabbageWidgetData::setProperty (widgetData, CabbageIdentifierIds::text, "");

        currentValueAsText = CabbageWidgetData::getProperty (widgetData, CabbageIdentifierIds::value).toString();
        const int index = stringItems.indexOf (currentValueAsText);
		owner->sendChannelStringDataToCsound(getChannel(), folderFiles[index].getFullPathName().getCharPointer());

        if (index != -1)  
            setSelectedItemIndex (index+1, dontSendNotification);
    }
    else
    {

        if (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::filetype).contains ("snaps"))
        {
            isPresetCombo = true;  
            getProperties().set("isPresetCombo", true);
            String presetName = CabbageWidgetData::getProperty(widgetData, CabbageIdentifierIds::value).toString();

            const int index = presets.indexOf(presetName);
            //don't send notification here, otherwise the saved session settings will be overwriten by the presets...
            setSelectedItemIndex ((index-1 >= 0 ? index : 0), dontSendNotification);
        }
        else
        {
            owner->sendChannelDataToCsound (getChannel(), getValue());
            setSelectedItemIndex (getValue() - 1, dontSendNotification);
        }
    }

}
//---------------------------------------------
CabbageComboBox::~CabbageComboBox()
{

}

void CabbageComboBox::addItemsToCombobox (ValueTree wData)
{
    Array<File> dirFiles;
    presets.clear();
    clear (dontSendNotification);
    folderFiles.clear();

    //load items from text file
    if (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::file).isNotEmpty())
    {
        String mfile = File (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::file)).loadFileAsString();
        StringArray lines = StringArray::fromLines (mfile);

        for (int i = 0; i < lines.size(); ++i)
        {
            addItem (lines[i], i + 1);
        }
    }

    //load items from text() list
    else if (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::filetype).isEmpty())
    {
        var items = CabbageWidgetData::getProperty (wData, CabbageIdentifierIds::text);
        PopupMenu subMenu;
        std::vector<StringArray> menus;
        int menuIndex = -1;
        for( int i = 0 ; i < items.size(); i++)
        {
            if(items[i].toString().contains("subM:"))
            {
                menus.push_back(StringArray());
                menuIndex++;
            }
            
            if(menus.size() > 0)
                menus[menuIndex].add(items[i]);
        }
        
        
        
        //if working with submenus....
        menuIndex = 1;
        if(menus.size()>0)
        {
            for( int i = 0 ; i < menus.size() ; i++)
            {
                subMenu.clear();
                for( int x = 1 ; x < menus[i].size() ; x++)
                {
                    subMenu.addItem(menuIndex, menus[i][x]);
                    menuIndex++;
                    if(x == menus[i].size() - 1){
                        const String subMenuName = menus[i][0].substring(6);
                        getRootMenu()->addSubMenu(subMenuName, subMenu);
                    }
                }
            }
        
        }
        else{
            for (int i = 0; i < items.size(); i++)
            {
                const String item  = items[i].toString();
                addItem (item, i + 1);
                stringItems.add (item);
            }
        }
    }
    //if dealing with preset files...
    else if ( CabbageWidgetData::getStringProp (wData, "filetype") == "*.snaps"
             || CabbageWidgetData::getStringProp (wData, "filetype") == ".snaps"
             || CabbageWidgetData::getStringProp (wData, "filetype") == "snaps") //load items from directory
    {
        const File fileName = File (getCsdFile()).withFileExtension (".snaps");

        if (fileName.existsAsFile())
        {
            std::unique_ptr<XmlElement> xmlElement = XmlDocument::parse (fileName);
            int itemIndex = 1;

            if (xmlElement)
                if (xmlElement->hasTagName ("CABBAGE_PRESETS"))
                {
                    forEachXmlChildElement (*xmlElement, e)
                    {
                        presets.add (e->getStringAttribute ("PresetName"));
                        addItem (e->getStringAttribute ("PresetName"), itemIndex++);
                    }
                }

            xmlElement = nullptr;
        }
    }
    else
    {
        
        const String workingDirToUse = CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::workingdir);

        if (workingDirToUse.isNotEmpty())
            pluginDir = File::getCurrentWorkingDirectory().getChildFile (workingDirToUse);
        else
            pluginDir = File::getCurrentWorkingDirectory();

        filetype = CabbageWidgetData::getStringProp (wData, "filetype");
        pluginDir.findChildFiles (dirFiles, 2, false, filetype);
        addItem ("Select..", 1);

        for (int i = 0; i < dirFiles.size(); ++i)
            folderFiles.add (dirFiles[i]);

        folderFiles.sort();

        for ( int i = 0; i < folderFiles.size(); i++)
        {
			stringItems.add(folderFiles[i].getFileNameWithoutExtension());
            addItem (folderFiles[i].getFileNameWithoutExtension(), i + 2);
        }

        setSelectedItemIndex(getNumItems()-1, dontSendNotification);

    }

    Justification justify (Justification::centred);

    if (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::align) == "left")
        justify = Justification::left;
    else if (CabbageWidgetData::getStringProp (wData, CabbageIdentifierIds::align) == "centre")
        justify = Justification::centred;
    else
        justify = Justification::right;

    setJustificationType (justify);
}

void CabbageComboBox::comboBoxChanged (ComboBox* combo) //this listener is only enabled when combo is loading presets or strings...
{
    if (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::filetype).contains ("snaps")
        || CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::filetype) == ("preset"))
    {
        String presetFilename;
        if (owner->isAudioUnit())
            presetFilename = File(getCsdFile()).withFileExtension(".snaps").getFullPathName();
        else
            presetFilename = owner->createNewGenericNameForPresetFile();
        
        owner->restorePluginStateFrom (presets[combo->getSelectedItemIndex()], presetFilename);
        owner->sendChannelStringDataToCsound (getChannel(), presets[combo->getSelectedItemIndex()]);
        CabbageWidgetData::setProperty (widgetData, CabbageIdentifierIds::value, presets[combo->getSelectedItemIndex()]);
    }
    else if (CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::channeltype).contains ("string"))
    {
        const String fileType = CabbageWidgetData::getStringProp (widgetData, CabbageIdentifierIds::filetype);
        const int index = combo->getSelectedItemIndex();

		if (fileType.isNotEmpty())
		{
			String test = folderFiles[index - 1].getFullPathName();
			owner->sendChannelStringDataToCsound(getChannel(), folderFiles[index - 1].getFullPathName().replaceCharacters("\\", "/"));
		}
        else
            owner->sendChannelStringDataToCsound (getChannel(), stringItems[index]);

    }
}

void CabbageComboBox::valueTreePropertyChanged (ValueTree& valueTree, const Identifier& prop)
{
    if (prop == CabbageIdentifierIds::value)
    {
        if (isPresetCombo == false)
        {
            if (isStringCombo == false)
            {
                const int mValue = CabbageWidgetData::getNumProp (valueTree, CabbageIdentifierIds::value);

                if (CabbageWidgetData::getNumProp (valueTree, CabbageIdentifierIds::update) == 1)
                    setSelectedItemIndex (mValue - 1, sendNotification);
                else
                    setSelectedItemIndex (mValue - 1, dontSendNotification);
            }
            else
            {
                currentValueAsText = CabbageWidgetData::getProperty (valueTree, CabbageIdentifierIds::value).toString();
                const int index = stringItems.indexOf (currentValueAsText);

                if (index != -1)
                    setSelectedItemIndex (index, dontSendNotification);

                CabbageWidgetData::setProperty (valueTree, CabbageIdentifierIds::value, currentValueAsText);
            }
        }
		else
		{
			currentValueAsText = CabbageWidgetData::getProperty(valueTree, CabbageIdentifierIds::value).toString();
			const int index = stringItems.indexOf(currentValueAsText);

			if (index != -1)
				setSelectedItemIndex(index, dontSendNotification);

			//CabbageWidgetData::setProperty(valueTree, CabbageIdentifierIds::value, currentValueAsText);
		}

    }

    else
    {
        const MessageManagerLock lock;
        handleCommonUpdates (this, valueTree);
        setColour (ComboBox::backgroundColourId, Colour::fromString (CabbageWidgetData::getStringProp (valueTree, CabbageIdentifierIds::colour)));
        setColour (ComboBox::textColourId, Colour::fromString (CabbageWidgetData::getStringProp (valueTree, CabbageIdentifierIds::fontcolour)));
        setColour (PopupMenu::backgroundColourId, Colour::fromString (CabbageWidgetData::getStringProp (valueTree, CabbageIdentifierIds::menucolour)));

        setTooltip (getCurrentPopupText (valueTree));

        if (workingDir != CabbageWidgetData::getStringProp (valueTree, CabbageIdentifierIds::workingdir) || prop == CabbageIdentifierIds::populate)
        {
            addItemsToCombobox (valueTree);
            workingDir = CabbageWidgetData::getStringProp (valueTree, CabbageIdentifierIds::workingdir);
        }


        if (CabbageWidgetData::getNumProp (valueTree, CabbageIdentifierIds::refreshfiles)==1)
        {
            CabbageWidgetData::setNumProp(valueTree, CabbageIdentifierIds::refreshfiles, 0);
            addItemsToCombobox (valueTree);
        }
    }

    repaint();
}
