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

#include "CabbageSettingsWindow.h"
//==============================================================================
static void addCustomListener(Array<PropertyComponent*> comps, CabbageSettingsWindow* owner)
{
    for( int i = 0; i < comps.size(); i++)
    {
        if(TextPropertyComponent* textProperty = dynamic_cast<TextPropertyComponent*>(comps[i]))
        {
            textProperty->addListener(owner);
        }
        else if(ColourPropertyComponent* colourProperty = dynamic_cast<ColourPropertyComponent*>(comps[i]))
        {
            colourProperty->addChangeListener(owner);
        }
        else if(CabbageFilePropertyComponent* fileComp = dynamic_cast<CabbageFilePropertyComponent*>(comps[i]))
        {
            fileComp->filenameComp.addListener(owner);
        }
    }
}

//static void addFilenameComponentListener(FilenameComponentListener* parent, PropertyComponent* propertyComponent)
//{
//    if(CabbageFilePropertyComponent* fileComp = dynamic_cast<CabbageFilePropertyComponent*>(propertyComponent))
//        fileComp->filenameComp.addListener(parent);
//
//}
//
//static void addColourListener(Array<PropertyComponent*> comps, CabbageSettingsWindow* owner)
//{
//    for( int i = 0; i < comps.size(); i++)
//    {
//        if(ColourPropertyComponent* colourProperty = dynamic_cast<ColourPropertyComponent*>(comps[i]))
//        {
//            colourProperty->addChangeListener(owner);
//        }
//    }
//}

CabbageSettingsWindow::CabbageSettingsWindow(CabbageSettings &settings, AudioDeviceSelectorComponent* audioDevice):
    Component(""),
    settings(settings),
    valueTree(settings.getValueTree()),
    audioSettingsButton("AudioSettingsButton"),
    miscSettingsButton("MiscSettingsButton"),
    colourSettingsButton("ColourSettingsButton"),
    audioDeviceSelector(audioDevice)
{
    addColourProperties();
    addMiscProperties();
    addAndMakeVisible (colourPanel);
    addAndMakeVisible (miscPanel);
    colourPanel.setVisible(false);
    miscPanel.setVisible(false);
    addAndMakeVisible(audioSettingsButton);
    addAndMakeVisible(miscSettingsButton);
    addAndMakeVisible(colourSettingsButton);
    addAndMakeVisible(audioDeviceSelector);
    audioSettingsButton.addListener(this);
    miscSettingsButton.addListener(this);
    colourSettingsButton.addListener(this);

    const Image audioSettingsImage = ImageCache::getFromMemory (CabbageBinaryData::AudioSettingsButton_png, CabbageBinaryData::AudioSettingsButton_pngSize);
    CabbageUtilities::setImagesForButton(&audioSettingsButton, audioSettingsImage);

    const Image miscSettingsImage = ImageCache::getFromMemory (CabbageBinaryData::MiscSettingsButton_png, CabbageBinaryData::MiscSettingsButton_pngSize);
    CabbageUtilities::setImagesForButton(&miscSettingsButton, miscSettingsImage);

    const Image colourSettingsImage = ImageCache::getFromMemory (CabbageBinaryData::ColourSettingsButton_png, CabbageBinaryData::ColourSettingsButton_pngSize);
    CabbageUtilities::setImagesForButton(&colourSettingsButton, colourSettingsImage);

}

void CabbageSettingsWindow::addColourProperties()
{
    Array<PropertyComponent*> editorProps, interfaceProps, consoleProps;

    for (int index = 0; index < valueTree.getChildWithName("Colours").getNumProperties(); ++index)
    {
        const String name = CabbageSettings::getColourPropertyName(valueTree, index);
        const Colour colour = CabbageSettings::getColourFromValueTree(valueTree, index, Colours::red);
        if(name.contains("Editor -"))
            editorProps.add (new ColourPropertyComponent(name, colour.toString()));
        else if(name.contains("Console -"))
            consoleProps.add (new ColourPropertyComponent(name, colour.toString()));
        else if(name.contains("Interface -"))
            interfaceProps.add (new ColourPropertyComponent(name, colour.toString()));
    }
    colourPanel.clear();
    addCustomListener(interfaceProps, this);
    addCustomListener(editorProps, this);
    addCustomListener(consoleProps, this);

    colourPanel.addSection("Interface", interfaceProps);
    colourPanel.addSection("Editor", editorProps);
    colourPanel.addSection("Console", consoleProps);

}

void CabbageSettingsWindow::addMiscProperties()
{
    Array<PropertyComponent*> props;

    showLastOpenedFileValue.setValue(settings.getUserSettings()->getIntValue("OpenMostRecentFileOnStartup"));
    showLastOpenedFileValue.addListener(this);
    alwaysOnTopValue.setValue(settings.getUserSettings()->getIntValue("SetAlwaysOnTop"));
    alwaysOnTopValue.addListener(this);
    compileOnSaveValue.setValue(settings.getUserSettings()->getIntValue("CompileOnSave"));
    compileOnSaveValue.addListener(this);

    props.add (new BooleanPropertyComponent(showLastOpenedFileValue, "Auto-load", "Auto-load last opened file"));
    props.add (new BooleanPropertyComponent(alwaysOnTopValue, "Plugin Window", "Always show plugin on top"));
    props.add (new BooleanPropertyComponent(compileOnSaveValue, "Compiling", "Compile on save"));

    props.add (new CabbageFilePropertyComponent("Csound manual dir.", true, false));
    props.add (new CabbageFilePropertyComponent("Cabbage plants dir.", true, false));
    props.add (new CabbageFilePropertyComponent("Cabbage examples dir.", true, false));

    const String sshAddress = settings.getUserSettings()->getValue("SSHAddress");
    props.add(new TextPropertyComponent(Value (sshAddress), "SSH Address", 200, false));

    const String sshHomeDir = settings.getUserSettings()->getValue("SSHHomeDir");
    props.add(new TextPropertyComponent(Value (sshHomeDir), "SSH Home Directory", 200, false));

    addCustomListener(props, this);
    miscPanel.clear();
    miscPanel.addProperties(props);
}

void CabbageSettingsWindow::textPropertyComponentChanged(TextPropertyComponent *comp)
{
    if(comp->getName()=="SSH Address")
        settings.getUserSettings()->setValue("SSHAddress", comp->getValue().toString());
    else if(comp->getName()=="SSH Home Directory")
        settings.getUserSettings()->setValue("SSHHomeDir", comp->getValue().toString());
}

void CabbageSettingsWindow::resized()
{
    Rectangle<int> r (getLocalBounds());
    audioSettingsButton.setBounds(10, 10, 60, 60);
    miscSettingsButton.setBounds(10, 80, 60, 60);
    colourSettingsButton.setBounds(10, 150, 60, 60);

    if(audioDeviceSelector)
        audioDeviceSelector->setBounds(100, 10, r.getWidth()-100, r.getHeight()-20);

    colourPanel.setBounds(100, 10, r.getWidth()-100, r.getHeight()-20);
    miscPanel.setBounds(100, 10, r.getWidth()-100, r.getHeight()-20);

}

void CabbageSettingsWindow::paint(Graphics& g)
{
    g.fillAll(Colour(147, 210, 0));
}
//=====================================================================
void CabbageSettingsWindow::valueChanged(Value& value)
{
    if(value.refersToSameSourceAs(showLastOpenedFileValue))
        settings.getUserSettings()->setValue("OpenMostRecentFileOnStartup", value.getValue().toString());
    else if(value.refersToSameSourceAs(alwaysOnTopValue))
        settings.getUserSettings()->setValue("SetAlwaysOnTop", value.getValue().toString());
    else if(value.refersToSameSourceAs(compileOnSaveValue))
        settings.getUserSettings()->setValue("CompileOnSave", value.getValue().toString());
    else if(value.refersToSameSourceAs(breakLinesValue))
        settings.getUserSettings()->setValue("IdentifiersBeforeLineBreak", value.getValue().toString());
}

void CabbageSettingsWindow::filenameComponentChanged (FilenameComponent* fileComponent)
{
    CabbageUtilities::debug(fileComponent->getName());
}

void CabbageSettingsWindow::buttonClicked(Button* button)
{
    if(button->getName()=="AudioSettingsButton")
    {
        audioDeviceSelector->setVisible(true);
        colourPanel.setVisible(false);
        miscPanel.setVisible(false);
    }
    else if(button->getName()=="ColourSettingsButton")
    {
        audioDeviceSelector->setVisible(false);
        colourPanel.setVisible(true);
        miscPanel.setVisible(false);
    }
    else if(button->getName()=="MiscSettingsButton")
    {
        audioDeviceSelector->setVisible(false);
        colourPanel.setVisible(false);
        miscPanel.setVisible(true);
    }

}

void CabbageSettingsWindow::changeListenerCallback(ChangeBroadcaster *source)
{
    if(ColourPropertyComponent* colourProperty = dynamic_cast<ColourPropertyComponent*>(source))
    {
        CabbageSettings::set(settings.getValueTree(), "Colours", colourProperty->getName(), colourProperty->getCurrentColourString());
    }
}