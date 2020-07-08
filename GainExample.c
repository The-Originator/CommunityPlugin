
#include "plugin-opi.h"

#include <vector>
#include <cstdio>

struct OpiGain : public OpiPlugin
{
    std::vector<char> stringBuffer;

    int nChannels = 0;
    float gain = 1;

    OpiGain(OpiCallback hostCallback, void * ptr)
    {
        dispatchToHost = hostCallback;
        dispatchToPlugin = &pluginDispatcher;
        ptrHost = ptr;
    }

    void process(OpiProcessInfo * procInfo)
    {
        assert(procInfo->processInfoSize == sizeof(OpiProcessInfo));
        
        int nFrames = procInfo->nFrames;

        uint64_t silenceMask = procInfo->inputs[0].silenceMask;
        uint64_t skipMask = silenceMask & procInfo->outputs[0].silenceMask;

        for(int c = 0; c < nChannels; ++c)
        {
            if((1<<c) & skipMask) continue;
            if((1<<c) & silenceMask)
            {
                for(int i = 0; i < nFrames; ++i)
                {
                    procInfo->outputs[0].channels[c][i] = 0;
                }
            }
            else
            {
                for(int i = 0; i < nFrames; ++i)
                {
                    procInfo->outputs[0].channels[c][i]
                        = gain * procInfo->inputs[0].channels[c][i];
                }
            }
        }
    }

    int configure(OpiConfig * config)
    {
        if(config->inBusChannels[0].nChannels
        == config->inBusChannels[1].nChannels)
        {
            nChannels = config->inBusChannels[0].nChannels;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    void setStringBuffer(const char * data, int size = -1)
    {
        if(size == -1) size = strlen(data);
        
        stringBuffer.resize(size + 1);
        memcpy(stringBuffer.data(), data, size);
        stringBuffer[size] = 0;
    }

    static intptr_t pluginDispatcher(
        struct OpiPlugin * ptr, int32_t op, int32_t idx, void*data)
    {
        OpiGain * plug = (OpiGain*) ptr;

        if(!op)
        {
            plug->process((OpiProcessInfo*) data);
            return 1;
        }
        
        switch(op)
        {
        case opiPlugDestroy: delete plug; return 1;

        case opiPlugNumInputs: return 1;    // one input
        case opiPlugNumOutputs: return 1;   // one output
        case opiPlugMaxChannels: return 2;  // up to 2 channels

        case opiPlugInEventMask: return 0;  // don't want events
        case opiPlugOutEventMask: return 0; // don't emit events

        case opiPlugGetLatency: return 0;   // no latency

        case opiPlugConfig: return plug->configure((OpiConfig*) data);
        case opiPlugReset: return 1;    // no-op

        case opiPlugEnable: return 1;   // no-op
        case opiPlugDisable: return 1;  // no-op

        case opiPlugOpenEdit: return 0;     // don't have editor
        case opiPlugCloseEdit: return 0;    // don't have editor

        case opiPlugSaveChunk: return 0;    // not implemented
        case opiPlugLoadChunk: return 0;    // not implemented
            
        case opiPlugNumParam: return 1; // one parameter
        
        case opiPlugGetParam:
            switch(idx)
            {
            case 0: *((float*)data) = plug->gain; return 1;
            default: return 0;
            }
            
        case opiPlugSetParam:
            switch(idx)
            {
            case 0:
                plug->gain = *((float*)data);
                if(plug->gain < 0) plug->gain = 0;
                if(plug->gain > 1) plug->gain = 1;
                return 1;
            default: return 0;
            }

        case opiPlugGetParamName:
            switch(idx)
            {
            case 0: plug->setStringBuffer("Gain"); break;
            default: return 0;
            }
            ((OpiString*)data)->data = plug->stringBuffer.data();
            ((OpiString*)data)->size = plug->stringBuffer.size();
            return 1;

        case opiPlugValueToString:
            {
                switch(idx)
                {
                case 0:
                    // this is very naive
                    plug->stringBuffer.resize(32); // alloc "enough" space
                    plug->stringBuffer.resize(
                        sprintf(plug->stringBuffer.data(),
                            "%.2f", ((OpiParamString*)data)->value));
                    break;
                default: return 0;
                }
                ((OpiParamString*)data)->data = plug->stringBuffer.data();
                ((OpiParamString*)data)->size = plug->stringBuffer.size();
                return 1;
            }
        case opiPlugStringToValue:
            switch(idx)
            {
            case 0:
                plug->setStringBuffer(
                    ((OpiString*)data)->data,
                    ((OpiString*)data)->size);
                // this is even more naive, but .. whatever
                sscanf(plug->stringBuffer.data(),
                    "%f", &(((OpiParamString*)data)->value));
                break;
            default: return 0;
            }
            return 1;

        default: return 0;
        }
    }
};

DLLEXPORT OpiPlugin * OpiPluginEntrypoint(OpiCallback hostCallback, void * hostPtr)
{
    return new OpiGain(hostCallback, hostPtr);
}
