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

#ifndef CABBAGEPLUGINPROCESSOR_H_INCLUDED
#define CABBAGEPLUGINPROCESSOR_H_INCLUDED

#include "CsoundPluginProcessor.h"
#include "../../Widgets/CabbageWidgetData.h"
#include "../../CabbageIds.h"
#include "../../Widgets/CabbageXYPad.h"

class CabbageAudioParameter;

class CabbagePluginProcessor : public CsoundPluginProcessor,
public Timer
{
public:

    class CabbageJavaClass  : public DynamicObject
    {

        CabbagePluginProcessor* owner;
    public:

        CabbageJavaClass (CabbagePluginProcessor* owner): owner (owner)
        {
            setMethod ("print", print);
        }

        static Identifier getClassName()   { return "Cabbage"; }

        static var print (const var::NativeFunctionArgs& args)
        {
            if (args.numArguments > 0)
                if (CabbageJavaClass* thisObject = dynamic_cast<CabbageJavaClass*> (args.thisObject.getObject()))
                    thisObject->owner->cabbageScriptGeneratedCode.add (args.arguments[0].toString());

            return var::undefined();
        }



        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CabbageJavaClass)
    };

    struct PlantImportStruct
    {
        String nsp, name, csoundCode;
        StringArray cabbageCode;
    };

	CabbagePluginProcessor (File inputFile, AudioChannelSet ins, AudioChannelSet outs);
    CabbagePluginProcessor (File inputFile, AudioChannelSet ins, AudioChannelSet outs, AudioChannelSet sidechain);
	void createCsound(File inputFile, bool shouldCreateParameters = true);
    ~CabbagePluginProcessor();

    ValueTree cabbageWidgets;
    void getChannelDataFromCsound() override;
    void triggerCsoundEvents() override;
    void setWidthHeight();
    bool addImportFiles (StringArray& lineFromCsd);
    void parseCsdFile (StringArray& linesFromCsd);
    // use this instead of AudioProcessor::addParameter
    void addCabbageParameter(CabbageAudioParameter* parameter);
    void createCabbageParameters();
    void updateWidgets (String csdText);
    void handleXmlImport (XmlElement* xml, StringArray& linesFromCsd);
    void getMacros (StringArray& csdText);
    void generateCabbageCodeFromJS (PlantImportStruct& importData, String text);
    void insertUDOCode (PlantImportStruct importData, StringArray& linesFromCsd);
    void insertPlantCode (StringArray& linesFromCsd);
    bool isWidgetPlantParent (StringArray linesFromCsd, int lineNumber);
    bool shouldClosePlant (StringArray linesFromCsd, int lineNumber);
    void setPluginName (String name) {    pluginName = name;  }
    String getPluginName() { return pluginName;  }
    void expandMacroText (String &line, ValueTree wData);
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void setCabbageParameter(String channel, float value);
    CabbageAudioParameter* getParameterForXYPad (String name);
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    //===== XYPad methods =========
    void addXYAutomator (CabbageXYPad* xyPad, ValueTree wData);
    void enableXYAutomator (String name, bool enable, Line<float> dragLine);
    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void setParametersFromXml (XmlElement* e);
    XmlElement savePluginState (String tag, File xmlFile = File(), String presetName="");
    void restorePluginState (XmlElement* xmlElement);
    //==============================================================================
    StringArray cabbageScriptGeneratedCode;
    Array<PlantImportStruct> plantStructs;

    int64 csdLastModifiedAt;
    void timerCallback() override;
	//uid needed for Cabbage host
	AudioProcessorGraph::NodeID nodeId;

	File getCsdFile()
	{
		return csdFile;
	}

	StringArray getCurrentCsdFileAsStringArray()
	{
		StringArray csdArray;
		csdArray.addLines(csdFile.loadFileAsString());
		return csdArray;
	}
    
    // use this instead of AudioProcessor::getParameters
    const OwnedArray<CabbageAudioParameter>& getCabbageParameters() const { return parameters; }
    
private:
    controlChannelInfo_s* csoundChanList;
    int numberOfLinesInPlantCode = 0;
    String pluginName;
    File csdFile;
    int linesToSkip = 0;
    NamedValueSet macroText;
    var macroNames;
    var macroStrings;
    bool xyAutosCreated = false;
    OwnedArray<XYPadAutomator> xyAutomators;
	int samplingRate = 44100;
    int samplesInBlock = 64;
	int screenWidth, screenHeight;
	bool isUnityPlugin = false;
    int automationMode = 0;
    OwnedArray<CabbageAudioParameter> parameters;

};


/*
 This is a thin shim around juce::AudioParameterFloat that allows us to use a subset of its interface
 while being able to intercept calls to the host for parameters that are non-automatable.
 */

class CabbageAudioParameter
{
    
public:
    CabbageAudioParameter(CabbagePluginProcessor* owner,
                          ValueTree wData,
                          Csound& csound,
                          const String& channelToUse,
                          const String& name,
                          float minValue,
                          float maxValue,
                          float def,
                          float incr,
                          float skew,
                          bool automatable = true,
                          const String& suffix = String())
    : parameter(new CabbageHostParameter(*this, owner, wData, csound, channelToUse, name, suffix, minValue, maxValue, def, incr, skew, isCombo(name))),
    widgetName(name),
    isAutomatable(automatable),
    owner(owner)
    {

    }
    
    ~CabbageAudioParameter()
    {
        if (!hostParameterReleased)
        {
            delete parameter;
        }
    }
    
    void setValue(float newValue)
    {
        parameter->setValue(newValue);
    }
    
    float getValue() const
    {
        return parameter->getValue();
    }
    
    void setValueNotifyingHost(float newValue)
    {
        if (isAutomatable)
        {
            parameter->setValueNotifyingHost(newValue);
        }
        else
        {
            setValue(newValue);
        }
    }
    
    void beginChangeGesture()
    {
        if (isAutomatable)
        {
            parameter->beginChangeGesture();
        }
    }
    
    void endChangeGesture()
    {
        if (isAutomatable)
        {
            parameter->endChangeGesture();
        }
    }
    
    AudioParameterFloat* releaseHostParameter()
    {
        hostParameterReleased = true;
        return parameter;
    }
    
    const NormalisableRange<float>& getNormalisableRange() const { return parameter->getNormalisableRange(); }
    
    const String getChannel() const { return parameter->getChannel(); }
    const String getWidgetName() { return widgetName; }
    bool getIsAutomatable() const { return isAutomatable; }
    
private:
    class CabbageHostParameter : public AudioParameterFloat
    {
    public:
        virtual ~CabbageHostParameter() { }
        float getValue() const override { return range.convertTo0to1(currentValue); }
        
        void setValue(float newValue) override
        {
            currentValue = isCombo ? juce::roundToInt(range.convertFrom0to1 (newValue)) : range.convertFrom0to1 (newValue);
            processor->setCabbageParameter(channel, currentValue);
        }
        
        String getText(float normalizedValue, int length) const override
        {
            // TODO: number of decimal places to display is hardcoded right now
            String asText(range.convertFrom0to1(normalizedValue), 3);
            
            if (length > 0 && asText.length() + suffix.length() > length)
            {
                asText = asText.substring(0, asText.length() - suffix.length());
            }
            
            asText += suffix;
            
            return asText;
        }
        
        float getValueForText(const String& text) const override
        {
            return text.dropLastCharacters(suffix.length()).getFloatValue();
        }
        
        const String& getChannel() const { return channel; }
        
    private:
        CabbageHostParameter(CabbageAudioParameter& owner,
                             CabbagePluginProcessor* proc,
                             ValueTree wData,
                             Csound& csound,
                             const String& channelToUse,
                             const String& name,
                             const String& suffixToUse,
                             float minValue,
                             float maxValue,
                             float def,
                             float incr,
                             float skew,
                             bool isCombo)
        : AudioParameterFloat(name, channelToUse, NormalisableRange<float>(minValue, maxValue, incr, skew), def),
        channel(channelToUse),
        suffix(makeSuffix(suffixToUse)),
        currentValue(def),
        isCombo(isCombo),
        owner(owner),
        processor(proc)
        {
            
        }
        
        const String channel;
        const String suffix { };
        float currentValue;
        bool isCombo = false;
        
        CabbageAudioParameter& owner;
        CabbagePluginProcessor* processor;
        
        String makeSuffix(const String& suffixToUse)
        {
            String newSuffix = "";
            
            if (suffixToUse.length() > 0)
            {
                newSuffix = " " + suffixToUse;
            }
            
            return newSuffix;
        }
        
        friend class CabbageAudioParameter;
    };
    
    CabbageHostParameter* parameter;
    bool hostParameterReleased = false;
    
    const String widgetName;
    const bool isAutomatable = true;
    
    CabbagePluginProcessor* owner;
    
    bool isCombo(const String name)
    {
        if(name.contains("combobox"))
            return true;
        return false;
    }
};

#endif  // CABBAGEPLUGINPROCESSOR_H_INCLUDED
