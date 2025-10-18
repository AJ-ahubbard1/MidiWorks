#pragma once
#include "../RtMidi/RtMidi.h"
#include "../MidiMessage/MidiMessage.h"
#include "MidiError.h"

namespace MidiInterface
{
    class MidiOut
    {
    public:
        MidiOut()
        {
            mPlayer = new RtMidiOut();
            mPlayer->setErrorCallback(&midiErrorCallback, nullptr);
            mNumPorts = mPlayer->getPortCount();
            if (mNumPorts == 0)
            {
                std::cout << "No midiout ports... opening a virtual port.\n";
                mPlayer->openVirtualPort("RtMidiOut");
                mNumPorts = mPlayer->getPortCount();
                if (mNumPorts > 0)
                {
                    std::cout << "Success!\n";
                }
            }
            fillPortNames();
            mPlayer->openPort(mPortNum);
        }

        ~MidiOut()
        {
            mPlayer->closePort();
            delete mPlayer;
        }

        void changePort(ubyte p)
        {
            mPlayer->closePort();
            mPortNum = p;
            mPlayer->openPort(mPortNum);
        }

        void sendMessage(MidiMessage mm)
        {
            mPlayer->sendMessage(mm.mData, 3);
        }

        unsigned int getNumPorts() const
        {
            return mNumPorts;
        }

        const std::vector<std::string>& getPortNames() const
        {
            return mPortNames;
        }

    private:
        ubyte mPortNum{0};
        RtMidiOut* mPlayer;
        ubyte mNumPorts{0};
        std::vector<std::string> mPortNames;

        void fillPortNames()
        {
            for (int i = 0; i < mNumPorts; i++)
            {
                mPortNames.push_back(mPlayer->getPortName(i));
            }
        }
    };
}
