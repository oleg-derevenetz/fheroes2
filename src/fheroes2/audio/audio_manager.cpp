/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "audio_manager.h"
#include "agg_file.h"
#include "audio.h"
#include "game.h"
#include "logging.h"
#include "m82.h"
#include "mus.h"
#include "settings.h"
#include "system.h"
#include "timing.h"
#include "tools.h"
#include "xmi.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <queue>
#include <string>
#include <utility>

namespace
{
    struct MusicFileType
    {
        explicit MusicFileType( const MUS::EXTERNAL_MUSIC_TYPE type_ )
            : type( type_ )
        {
            // Do nothing.
        }

        MUS::EXTERNAL_MUSIC_TYPE type = MUS::EXTERNAL_MUSIC_TYPE::WIN_VERSION;

        std::array<std::string, 3> extension{ ".ogg", ".mp3", ".flac" };
    };

    const std::string externalMusicDirectory( "music" );

    std::vector<std::string> getMusicDirectories()
    {
        std::vector<std::string> directories;
        for ( const std::string & dir : Settings::GetRootDirs() ) {
            std::string fullDirectoryPath = System::ConcatePath( dir, externalMusicDirectory );
            if ( System::IsDirectory( fullDirectoryPath ) ) {
                directories.emplace_back( std::move( fullDirectoryPath ) );
            }
        }

        return directories;
    }

    bool findMusicFile( const std::vector<std::string> & directories, const std::string & fileName, std::string & fullPath )
    {
        for ( const std::string & dir : directories ) {
            ListFiles musicFilePaths;
            musicFilePaths.ReadDir( dir, fileName, false );
            if ( musicFilePaths.empty() ) {
                continue;
            }

            std::string correctFilePath = System::ConcatePath( dir, fileName );
            correctFilePath = StringLower( correctFilePath );

            for ( std::string & path : musicFilePaths ) {
                const std::string temp = StringLower( path );
                if ( temp == correctFilePath ) {
                    // Avoid string copy.
                    std::swap( fullPath, path );
                    return true;
                }
            }
        }

        return false;
    }

    std::string getExternalMusicFile( const int musicTrackId, const std::vector<std::string> & directories, MusicFileType & musicType )
    {
        if ( directories.empty() ) {
            // Nothing to search.
            return {};
        }

        std::string fullPath;

        std::string fileName = MUS::getFileName( musicTrackId, musicType.type, musicType.extension[0].c_str() );
        if ( findMusicFile( directories, fileName, fullPath ) ) {
            return fullPath;
        }

        fheroes2::replaceStringEnding( fileName, musicType.extension[0].c_str(), musicType.extension[1].c_str() );
        if ( findMusicFile( directories, fileName, fullPath ) ) {
            // Swap extensions to improve cache hit.
            std::swap( musicType.extension[0], musicType.extension[1] );
            return fullPath;
        }

        fheroes2::replaceStringEnding( fileName, musicType.extension[1].c_str(), musicType.extension[2].c_str() );
        if ( findMusicFile( directories, fileName, fullPath ) ) {
            // Swap extensions to improve cache hit.
            std::swap( musicType.extension[0], musicType.extension[2] );
            return fullPath;
        }

        // Looks like music file does not exist.
        return {};
    }

    struct ChannelAudioLoopEffectInfo : public AudioManager::AudioLoopEffectInfo
    {
        ChannelAudioLoopEffectInfo() = default;

        ChannelAudioLoopEffectInfo( const AudioLoopEffectInfo & info, const int channelId_ )
            : AudioLoopEffectInfo( info )
            , channelId( channelId_ )
        {
            // Do nothing.
        }

        int channelId{ -1 };
    };

    std::vector<uint8_t> getDataFromAggFile( const std::string & key, const bool ignoreExpansion );

    void LoadWAV( int m82, std::vector<uint8_t> & v )
    {
        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) )
        const std::vector<uint8_t> & body = getDataFromAggFile( M82::GetString( m82 ), false );

        if ( !body.empty() ) {
            // create WAV format
            StreamBuf wavHeader( 44 );
            wavHeader.putLE32( 0x46464952 ); // RIFF
            wavHeader.putLE32( static_cast<uint32_t>( body.size() ) + 0x24 ); // size
            wavHeader.putLE32( 0x45564157 ); // WAVE
            wavHeader.putLE32( 0x20746D66 ); // FMT
            wavHeader.putLE32( 0x10 ); // size_t
            wavHeader.putLE16( 0x01 ); // format
            wavHeader.putLE16( 0x01 ); // channels
            wavHeader.putLE32( 22050 ); // samples
            wavHeader.putLE32( 22050 ); // byteper
            wavHeader.putLE16( 0x01 ); // align
            wavHeader.putLE16( 0x08 ); // bitsper
            wavHeader.putLE32( 0x61746164 ); // DATA
            wavHeader.putLE32( static_cast<uint32_t>( body.size() ) ); // size

            v.reserve( body.size() + 44 );
            v.assign( wavHeader.data(), wavHeader.data() + 44 );
            v.insert( v.begin() + 44, body.begin(), body.end() );
        }
    }

    void LoadMID( int xmi, std::vector<uint8_t> & v )
    {
        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) )
        const std::vector<uint8_t> & body = getDataFromAggFile( XMI::GetString( xmi ), xmi >= XMI::MIDI_ORIGINAL_KNIGHT );

        if ( !body.empty() ) {
            v = Music::Xmi2Mid( body );
        }
    }

    std::map<int, std::vector<uint8_t>> wavDataCache;
    std::map<int, std::vector<uint8_t>> MIDDataCache;

    const std::vector<uint8_t> & GetWAV( int m82 )
    {
        std::vector<uint8_t> & v = wavDataCache[m82];
        if ( v.empty() ) {
            LoadWAV( m82, v );
        }

        return v;
    }

    const std::vector<uint8_t> & GetMID( int xmi )
    {
        std::vector<uint8_t> & v = MIDDataCache[xmi];
        if ( v.empty() ) {
            LoadMID( xmi, v );
        }

        return v;
    }

    void PlaySoundInternally( const int m82, const int soundVolume );
    void PlayMusicInternally( const int mus, const MusicSource musicType, const bool loop );
    void playLoopSoundsInternally( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects, const int soundVolume,
                                   const bool is3DAudioEnabled );

    // SDL MIDI player is a single threaded library which requires a lot of time to start playing some long midi compositions.
    // This leads to a situation of a short application freeze while a hero crosses terrains or ending a battle.
    // The only way to avoid this is to fire MIDI requests asynchronously and synchronize them if needed.
    class AsyncSoundManager : public fheroes2::AsyncManager
    {
    public:
        void pushMusic( const int musicId, const MusicSource musicType, const bool isLooped )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            while ( !_musicTasks.empty() ) {
                _musicTasks.pop();
            }

            _musicTasks.emplace( musicId, musicType, isLooped );
            notifyThread();
        }

        void pushSound( const int m82Sound, const int soundVolume )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _soundTasks.emplace( m82Sound, soundVolume );
            notifyThread();
        }

        void pushLoopSound( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> vols, const int soundVolume, const bool is3DAudioEnabled )
        {
            _createThreadIfNeeded();

            std::lock_guard<std::mutex> mutexLock( _mutex );

            _loopSoundTasks.emplace( std::move( vols ), soundVolume, is3DAudioEnabled );
            notifyThread();
        }

        void sync()
        {
            std::lock_guard<std::mutex> mutexLock( _mutex );

            while ( !_musicTasks.empty() ) {
                _musicTasks.pop();
            }

            while ( !_soundTasks.empty() ) {
                _soundTasks.pop();
            }

            while ( !_loopSoundTasks.empty() ) {
                _loopSoundTasks.pop();
            }
        }

        // This mutex is used to avoid access to global objects and classes related to SDL Mixer.
        std::mutex & resourceMutex()
        {
            return _resourceMutex;
        }

    private:
        struct MusicTask
        {
            MusicTask( const int musicId_, const MusicSource musicType_, const bool isLooped_ )
                : musicId( musicId_ )
                , musicType( musicType_ )
                , isLooped( isLooped_ )
            {}

            int musicId;
            MusicSource musicType;
            bool isLooped;
        };

        struct SoundTask
        {
            SoundTask( const int m82Sound_, const int soundVolume_ )
                : m82Sound( m82Sound_ )
                , soundVolume( soundVolume_ )
            {}

            int m82Sound;
            int soundVolume;
        };

        struct LoopSoundTask
        {
            LoopSoundTask( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> effects, const int soundVolume_, const bool is3DAudioOn )
                : soundEffects( std::move( effects ) )
                , soundVolume( soundVolume_ )
                , is3DAudioEnabled( is3DAudioOn )
            {
                // Do nothing.
            }

            std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects;
            int soundVolume;
            bool is3DAudioEnabled;
        };

        std::queue<MusicTask> _musicTasks;
        std::queue<SoundTask> _soundTasks;
        std::queue<LoopSoundTask> _loopSoundTasks;

        std::mutex _resourceMutex;

        void doStuff() override
        {
            if ( !_soundTasks.empty() ) {
                const SoundTask soundTask = _soundTasks.back();
                _soundTasks.pop();

                _mutex.unlock();

                PlaySoundInternally( soundTask.m82Sound, soundTask.soundVolume );
            }
            else if ( !_loopSoundTasks.empty() ) {
                LoopSoundTask loopSoundTask = _loopSoundTasks.back();
                _loopSoundTasks.pop();

                _mutex.unlock();

                playLoopSoundsInternally( std::move( loopSoundTask.soundEffects ), loopSoundTask.soundVolume, loopSoundTask.is3DAudioEnabled );
            }
            else if ( !_musicTasks.empty() ) {
                const MusicTask musicTask = _musicTasks.back();

                while ( !_musicTasks.empty() ) {
                    _musicTasks.pop();
                }

                _mutex.unlock();

                PlayMusicInternally( musicTask.musicId, musicTask.musicType, musicTask.isLooped );
            }
            else {
                _runFlag = 0;

                _mutex.unlock();
            }
        }
    };

    std::map<M82::SoundType, std::vector<ChannelAudioLoopEffectInfo>> currentAudioLoopEffects;

    fheroes2::AGGFile g_midiHeroes2AGG;
    fheroes2::AGGFile g_midiHeroes2xAGG;

    std::vector<uint8_t> getDataFromAggFile( const std::string & key, const bool ignoreExpansion )
    {
        if ( !ignoreExpansion && g_midiHeroes2xAGG.isGood() ) {
            // Make sure that the below container is not const and not a reference
            // so returning it from the function will invoke a move constructor instead of copy constructor.
            std::vector<uint8_t> buf = g_midiHeroes2xAGG.read( key );
            if ( !buf.empty() )
                return buf;
        }

        return g_midiHeroes2AGG.read( key );
    }

    AsyncSoundManager g_asyncSoundManager;

    void PlaySoundInternally( const int m82, const int soundVolume )
    {
        std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "Try to play sound " << M82::GetString( m82 ) )

        const std::vector<uint8_t> & v = GetWAV( m82 );
        if ( v.empty() ) {
            return;
        }

        const int channelId = Mixer::Play( &v[0], static_cast<uint32_t>( v.size() ), -1, false );
        if ( channelId < 0 ) {
            // Failed to get a free channel.
            return;
        }

        Mixer::Pause( channelId );
        Mixer::setVolume( channelId, 100 * soundVolume / 10 );
        Mixer::Resume( channelId );
    }

    uint64_t getMusicUID( const int trackId, const MusicSource musicType )
    {
        assert( trackId != MUS::UNUSED && trackId != MUS::UNKNOWN && trackId >= 0 );

        return ( static_cast<uint64_t>( musicType ) << 32 ) + static_cast<uint64_t>( trackId );
    }

    void PlayMusicInternally( const int mus, const MusicSource musicType, const bool loop )
    {
        // Make sure that the music track is valid.
        assert( mus != MUS::UNUSED && mus != MUS::UNKNOWN );

        std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

        if ( Game::CurrentMusic() == mus && Music::isPlaying() ) {
            return;
        }

        if ( musicType == MUSIC_EXTERNAL ) {
            const std::vector<std::string> & musicDirectories = getMusicDirectories();

            // To avoid extra I/O calls to data storage it might be useful to remember the last successful type of music and try to search for it next time.
            static std::array<MusicFileType, 3> musicFileTypes{ MusicFileType( MUS::EXTERNAL_MUSIC_TYPE::DOS_VERSION ),
                                                                MusicFileType( MUS::EXTERNAL_MUSIC_TYPE::WIN_VERSION ),
                                                                MusicFileType( MUS::EXTERNAL_MUSIC_TYPE::MAPPED ) };

            std::string filename;

            for ( size_t i = 0; i < musicFileTypes.size(); ++i ) {
                filename = getExternalMusicFile( mus, musicDirectories, musicFileTypes[i] );
                if ( !filename.empty() ) {
                    if ( i > 0 ) {
                        // Swap music types to improve cache hit.
                        std::swap( musicFileTypes[0], musicFileTypes[i] );
                    }
                    break;
                }
            }

            if ( filename.empty() ) {
                DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Cannot find a file for " << mus << " track." )
            }
            else {
                Music::Play( getMusicUID( mus, musicType ), filename, loop );

                Game::SetCurrentMusic( mus );

                DEBUG_LOG( DBG_ENGINE, DBG_TRACE, MUS::getFileName( mus, MUS::EXTERNAL_MUSIC_TYPE::MAPPED, " " ) )

                return;
            }
        }

        // Check if music needs to be pulled from HEROES2X
        int xmi = XMI::UNKNOWN;
        if ( musicType == MUSIC_MIDI_EXPANSION ) {
            xmi = XMI::FromMUS( mus, g_midiHeroes2xAGG.isGood() );
        }

        if ( XMI::UNKNOWN == xmi ) {
            xmi = XMI::FromMUS( mus, false );
        }

        if ( XMI::UNKNOWN != xmi ) {
            const std::vector<uint8_t> & v = GetMID( xmi );
            if ( !v.empty() ) {
                Music::Play( getMusicUID( mus, musicType ), v, loop );

                Game::SetCurrentMusic( mus );
            }
        }
        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) )
    }

    void playLoopSoundsInternally( std::map<M82::SoundType, std::vector<AudioManager::AudioLoopEffectInfo>> soundEffects, const int soundVolume,
                                   const bool is3DAudioEnabled )
    {
        std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

        if ( soundVolume == 0 ) {
            // The volume is 0. Remove all existing sound effects.
            for ( const auto & audioEffectPair : currentAudioLoopEffects ) {
                const std::vector<ChannelAudioLoopEffectInfo> & existingEffects = audioEffectPair.second;

                for ( const ChannelAudioLoopEffectInfo & info : existingEffects ) {
                    if ( Mixer::isPlaying( info.channelId ) ) {
                        Mixer::setVolume( info.channelId, 0 );
                        Mixer::Stop( info.channelId );
                    }
                }
            }

            currentAudioLoopEffects.clear();
            return;
        }

        std::map<M82::SoundType, std::vector<ChannelAudioLoopEffectInfo>> tempAudioLoopEffects;
        std::swap( tempAudioLoopEffects, currentAudioLoopEffects );

        // First find channels with existing sounds and just update them.
        for ( auto iter = soundEffects.begin(); iter != soundEffects.end(); ) {
            const M82::SoundType soundType = iter->first;
            std::vector<AudioManager::AudioLoopEffectInfo> & effects = iter->second;
            assert( !effects.empty() );

            auto foundSoundTypeIter = tempAudioLoopEffects.find( soundType );
            if ( foundSoundTypeIter == tempAudioLoopEffects.end() ) {
                // This sound type does not exist.
                ++iter;
                continue;
            }

            std::vector<ChannelAudioLoopEffectInfo> & existingEffects = foundSoundTypeIter->second;

            size_t elementSize = std::min( effects.size(), existingEffects.size() );
            while ( elementSize > 0 ) {
                --elementSize;

                currentAudioLoopEffects[soundType].emplace_back( existingEffects.back() );
                existingEffects.pop_back();

                ChannelAudioLoopEffectInfo & currentInfo = currentAudioLoopEffects[soundType].back();
                currentInfo = { effects.back(), currentInfo.channelId };
                effects.pop_back();

                if ( Mixer::isPlaying( currentInfo.channelId ) ) {
                    Mixer::setVolume( currentInfo.channelId, currentInfo.volumePercentage * soundVolume / 10 );

                    if ( is3DAudioEnabled ) {
                        Mixer::applySoundEffect( currentInfo.channelId, currentInfo.angle, currentInfo.volumePercentage );
                    }
                }
            }

            if ( existingEffects.empty() ) {
                tempAudioLoopEffects.erase( foundSoundTypeIter );
            }

            if ( effects.empty() ) {
                iter = soundEffects.erase( iter );
            }
            else {
                ++iter;
            }
        }

        for ( const auto & audioEffectPair : tempAudioLoopEffects ) {
            const std::vector<ChannelAudioLoopEffectInfo> & existingEffects = audioEffectPair.second;

            for ( const ChannelAudioLoopEffectInfo & info : existingEffects ) {
                if ( Mixer::isPlaying( info.channelId ) ) {
                    Mixer::setVolume( info.channelId, 0 );
                    Mixer::Stop( info.channelId );
                }
            }
        }

        tempAudioLoopEffects.clear();

        // Add new sound effects.
        for ( const auto & audioEffectPair : soundEffects ) {
            const M82::SoundType soundType = audioEffectPair.first;
            const std::vector<AudioManager::AudioLoopEffectInfo> & effects = audioEffectPair.second;
            assert( !effects.empty() );

            for ( const AudioManager::AudioLoopEffectInfo & info : effects ) {
                // It is a new sound effect. Register and play it.
                const std::vector<uint8_t> & audioData = GetWAV( soundType );
                if ( audioData.empty() ) {
                    // Looks like nothing to play. Ignore it.
                    continue;
                }

                int channelId = -1;
                if ( is3DAudioEnabled ) {
                    channelId = Mixer::PlayFromDistance( &audioData[0], static_cast<uint32_t>( audioData.size() ), -1, true, info.angle, info.volumePercentage );
                }
                else {
                    channelId = Mixer::Play( &audioData[0], static_cast<uint32_t>( audioData.size() ), -1, true );
                }

                if ( channelId < 0 ) {
                    // Unable to play this audio. It is probably an invalid audio sample.
                    continue;
                }

                // Adjust channel based on given parameters.

                // TODO: this is very hacky way. We should not do this. For example in 3D audio mode when a hero moves alongside beach it is noticeable that ocean sounds
                //       are 'jumping' in volume. Instead of such approach we need to get free channel ID which will be used for playback. Set volume for it and then
                //       start playing. Such logic must be implemented within Audio related code.
                //       As an alternative solution: we can use channel IDs which we freed in the previous step. However, be careful with synchronization for audio
                //       access.
                Mixer::Pause( channelId );
                Mixer::setVolume( channelId, info.volumePercentage * soundVolume / 10 );
                Mixer::Resume( channelId );

                currentAudioLoopEffects[soundType].emplace_back( info, channelId );

                DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "Playing sound " << M82::GetString( soundType ) )
            }
        }
    }
}

namespace AudioManager
{
    AudioInitializer::AudioInitializer( const std::string & originalAGGFilePath, const std::string & expansionAGGFilePath )
    {
        if ( Audio::isValid() ) {
            Mixer::SetChannels( 32 );
            // Set the volume for all channels to 0. This is required to avoid random volume spikes at the beginning of the game.
            Mixer::setVolume( -1, 0 );

            Music::setVolume( 100 * Settings::Get().MusicVolume() / 10 );
            Music::SetFadeInMs( 900 );
        }

        assert( !originalAGGFilePath.empty() );
        if ( !g_midiHeroes2AGG.open( originalAGGFilePath ) ) {
            VERBOSE_LOG( "Failed to open HEROES2.AGG file for audio playback." );
        }

        if ( !expansionAGGFilePath.empty() && !g_midiHeroes2xAGG.open( expansionAGGFilePath ) ) {
            VERBOSE_LOG( "Failed to open HEROES2X.AGG file for audio playback." );
        }
    }

    AudioInitializer::~AudioInitializer()
    {
        g_asyncSoundManager.sync();

        wavDataCache.clear();
        MIDDataCache.clear();
        currentAudioLoopEffects.clear();
    }

    void playLoopSounds( std::map<M82::SoundType, std::vector<AudioLoopEffectInfo>> soundEffects, bool asyncronizedCall )
    {
        if ( !Audio::isValid() ) {
            return;
        }

        const Settings & conf = Settings::Get();

        if ( asyncronizedCall ) {
            g_asyncSoundManager.pushLoopSound( std::move( soundEffects ), conf.SoundVolume(), conf.is3DAudioEnabled() );
        }
        else {
            g_asyncSoundManager.sync();
            playLoopSoundsInternally( std::move( soundEffects ), conf.SoundVolume(), conf.is3DAudioEnabled() );
        }
    }

    void PlaySound( int m82, bool asyncronizedCall )
    {
        if ( m82 == M82::UNKNOWN ) {
            return;
        }

        if ( !Audio::isValid() ) {
            return;
        }

        if ( asyncronizedCall ) {
            g_asyncSoundManager.pushSound( m82, Settings::Get().SoundVolume() );
        }
        else {
            g_asyncSoundManager.sync();
            PlaySoundInternally( m82, Settings::Get().SoundVolume() );
        }
    }

    void PlayMusic( int mus, bool loop, bool asyncronizedCall )
    {
        if ( MUS::UNUSED == mus || MUS::UNKNOWN == mus ) {
            return;
        }

        if ( !Audio::isValid() ) {
            return;
        }

        if ( asyncronizedCall ) {
            g_asyncSoundManager.pushMusic( mus, Settings::Get().MusicType(), loop );
        }
        else {
            g_asyncSoundManager.sync();
            PlayMusicInternally( mus, Settings::Get().MusicType(), loop );
        }
    }

    void ResetAudio()
    {
        if ( !Audio::isValid() ) {
            // Nothing to reset as an audio device is not even initialized.
            return;
        }

        g_asyncSoundManager.sync();

        std::lock_guard<std::mutex> mutexLock( g_asyncSoundManager.resourceMutex() );

        Music::Stop();
        Mixer::Stop();

        currentAudioLoopEffects.clear();
    }
}