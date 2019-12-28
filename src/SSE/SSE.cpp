#include "stdafx.h"
#include <vector>

SSE::SSE(void* hWnd, dword samplesPerSec, word channels, word bitsPerSample, word maxFX)
    : _hWnd((HWND)hWnd)
    , _samplesPerSec(samplesPerSec)
    , _channels(channels)
    , _bitsPerSample(bitsPerSample)
    , _maxSound(maxFX)
{
    if (Mix_OpenAudioDevice(samplesPerSec, SDL_AUDIO_MASK_SIGNED | (bitsPerSample & SDL_AUDIO_MASK_BITSIZE), channels, 1024, nullptr, 0) < 0)
    {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    }

    Mix_AllocateChannels(maxFX);
}

SSE::~SSE()
{
}

HRESULT SSE::CreateFX(FX** ppFX, char* file, dword samplesPerSec, word channels, word bitsPerSample)
{
    if (!ppFX)
        return SSE_INVALIDPARAM;

    _soundObjList.emplace_back(FX());
    *ppFX = &_soundObjList.back();
    return (*ppFX)->Create(this, file, samplesPerSec, channels, bitsPerSample);
}

HRESULT SSE::CreateMidi(MIDI** ppMidi, char* file)
{
    if (!ppMidi)
        return SSE_INVALIDPARAM;

    _musicObjList.emplace_back(MIDI());
    *ppMidi = &_musicObjList.back();
    return (*ppMidi)->Create(this, file);
}

HRESULT SSE::EnableDS()
{
    return SSE_OK;
}

HRESULT SSE::DisableDS()
{
    return SSE_OK;
}

FX::FX()
{
    memset(&_fxData, 0, sizeof(_fxData));
}

FX::~FX()
{
    Free();
}

HRESULT	FX::Create(SSE* pSSE, char* file, dword samplesPerSec, word channels, word bitsPerSample)
{
    _digitalData.pSSE = pSSE;
    SetFormat(samplesPerSec, channels, bitsPerSample);

    if (file)
        Load(file);

    return SSE_OK;
}

bool FX::StopPriority(dword flags)
{
    return false;
}

long FX::Release()
{
    return 0;
}

HRESULT FX::Play(dword dwFlags, long pan)
{
    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    // TODO: Panning
    if (dwFlags & DSBPLAY_SETPAN)
        SetPan(pan);

    if (_digitalData.fNoStop && Mix_Playing(_digitalData.channel) &&
        Mix_GetChunk(_digitalData.channel) == _fxData.pBuffer[0])
        return SSE_OK;

    _digitalData.fNoStop = (dwFlags & DSBPLAY_NOSTOP);
    _digitalData.channel = Mix_PlayChannel(-1, _fxData.pBuffer[0], dwFlags & DSBPLAY_LOOPING ? -1 : 0);
    _digitalData.time = timeGetTime();
    return _digitalData.channel < 0 ? SSE_CANNOTPLAY : SSE_OK;
}

HRESULT FX::Stop()
{
    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    if (Mix_GetChunk(_digitalData.channel) == _fxData.pBuffer[0])
        Mix_HaltChannel(_digitalData.channel);
    return SSE_OK;
}

HRESULT FX::Pause()
{
    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    if (Mix_GetChunk(_digitalData.channel) == _fxData.pBuffer[0])
        Mix_Pause(_digitalData.channel);
    return SSE_OK;
}

HRESULT FX::Resume()
{
    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    if (Mix_GetChunk(_digitalData.channel) == _fxData.pBuffer[0])
        Mix_Resume(_digitalData.channel);
    return SSE_OK;
}

HRESULT FX::GetVolume(long* pVolume)
{
    if (!pVolume)
        return SSE_INVALIDPARAM;

    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    *pVolume = Mix_VolumeChunk(_fxData.pBuffer[0], -1);
    return SSE_OK;
}

HRESULT FX::SetVolume(long volume)
{
    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    Mix_VolumeChunk(_fxData.pBuffer[0], volume);
    return SSE_OK;
}

HRESULT FX::GetPan(long* pPan)
{
    if (!pPan)
        return SSE_INVALIDPARAM;

    return SSE_OK;
}

HRESULT FX::SetPan(long pan)
{
    return SSE_OK;
}

HRESULT FX::Load(const char* file)
{
    _digitalData.file = file;

    Uint8* buf = (Uint8*)SDL_LoadFile(file, &_fxData.bufferSize);
    _fxData.pBuffer[0] = Mix_QuickLoad_RAW(buf, _fxData.bufferSize);
    return SSE_OK;
}

HRESULT FX::Fusion(const FX** Fx, long NumFx)
{
    for (long i = 0; i < NumFx; i++)
    {
        if (!Fx[i] || !Fx[i]->_fxData.pBuffer[0])
            return SSE_INVALIDPARAM;
    }

    Free();

    for (long i = 0; i < NumFx; i++)
        _fxData.bufferSize += Fx[i]->_fxData.bufferSize;
    Uint8* buf = (Uint8*)SDL_malloc(_fxData.bufferSize);
    size_t pos = 0;
    for (long i = 0; i < NumFx; i++)
    {
        memcpy(buf + pos, Fx[i]->_fxData.pBuffer[0]->abuf, Fx[i]->_fxData.bufferSize);
        pos += Fx[i]->_fxData.bufferSize;
    }
    _fxData.pBuffer[0] = Mix_QuickLoad_RAW(buf, _fxData.bufferSize);
    return SSE_OK;
}

HRESULT FX::Fusion(const FX* Fx, long* Von, long* Bis, long NumFx)
{
    if (!Fx || !Fx->_fxData.pBuffer[0])
        return SSE_INVALIDPARAM;

    Free();

    for (long i = 0; i < NumFx; i++)
        _fxData.bufferSize += Bis[i] - Von[i];
    Uint8* buf = (Uint8*)SDL_malloc(_fxData.bufferSize);
    size_t pos = 0;
    for (long i = 0; i < NumFx; i++)
    {
        memcpy(buf + pos, Fx->_fxData.pBuffer[0]->abuf + Von[i], Bis[i] - Von[i]);
        pos += Bis[i] - Von[i];
    }
    _fxData.pBuffer[0] = Mix_QuickLoad_RAW(buf, _fxData.bufferSize);
    return SSE_OK;
}

HRESULT FX::Tokenize(__int64 Token, long* Von, long* Bis, long& rcAnzahl)
{
    if (!_fxData.pBuffer[0] || _fxData.bufferSize < sizeof(__int64))
        return SSE_NOSOUNDLOADED;

    size_t count = 0;
    Von[count++] = 0;
    Uint8* ptr = _fxData.pBuffer[0]->abuf;
    for (size_t i = 0; i < _fxData.bufferSize - 7; i++)
    {
        if (*(__int64*)ptr == Token)
        {
            Bis[count - 1] = ((i - 1) & 0xFFFFFE) + 2;
            Von[count++] = (i + 8) & 0xFFFFFE;
        }
    }
    Bis[count - 1] = (i - 1);
    rcAnzahl = count;
    return SSE_OK;
}

FX** FX::Tokenize(__int64 Token, long& rcAnzahl)
{
    if (!_fxData.pBuffer[0] || _fxData.bufferSize < sizeof(__int64))
        return nullptr;

    std::vector<size_t> slices;
    Uint8* ptr = _fxData.pBuffer[0]->abuf;
    for (size_t i = 0; i < _fxData.bufferSize - 7; i++)
    {
        if (*(__int64*)ptr == Token)
            slices.push_back(i);
    }
    slices.push_back(_fxData.bufferSize);

    FX** pFX = new FX*[slices.size()];
    size_t pos = 0;
    for (size_t i = 0; i < slices.size(); i++)
    {
        _digitalData.pSSE->CreateFX(&pFX[i]);

        size_t size = slices[i] - pos;
        Uint8* buf = (Uint8*)SDL_malloc(size);
        memcpy(buf, _fxData.pBuffer[0]->abuf + pos, size);
        pFX[i]->_fxData.bufferSize = size;
        pFX[i]->_fxData.pBuffer[0] = Mix_QuickLoad_RAW(buf, size);
        pos = slices[i];
    }
    rcAnzahl = slices.size();
    return pFX;
}

HRESULT FX::Free()
{
    if (_fxData.pBuffer[0])
    {
        void* buf = _fxData.pBuffer[0]->abuf;
        Mix_FreeChunk(_fxData.pBuffer[0]);
        SDL_free(buf);
    }
    _digitalData.file.clear();
    _fxData.bufferSize = 0;
    return SSE_OK;
}

HRESULT FX::GetStatus(dword* pStatus)
{
    if (!pStatus)
        return SSE_INVALIDPARAM;

    if (!_fxData.pBuffer[0])
        return SSE_NOSOUNDLOADED;

    *pStatus = 0;
    if (Mix_Playing(_digitalData.channel) && Mix_GetChunk(_digitalData.channel) == _fxData.pBuffer[0])
    {
        *pStatus |= DSBSTATUS_PLAYING;

        if (_digitalData.fNoStop)
            *pStatus |= DSBSTATUS_LOOPING;
    }

    return SSE_OK;
}

bool FX::IsMouthOpen(long PreTime)
{
    if (!_fxData.pBuffer[0])
        return false;

    if (!Mix_Playing(_digitalData.channel) ||
        Mix_GetChunk(_digitalData.channel) != _fxData.pBuffer[0])
        return false;

    dword pos = 22050 * (timeGetTime() - _digitalData.time + PreTime) / 1000;
    if (pos * sizeof(Uint16) + 2000 >= _fxData.bufferSize)
        return false;

    Uint16* sampleBuf = ((Uint16*)_fxData.pBuffer[0]->abuf) + pos;
    return abs(*sampleBuf) > 512
        || abs(sampleBuf[100]) > 512
        || abs(sampleBuf[200]) > 512
        || abs(sampleBuf[400]) > 512
        || abs(sampleBuf[560]) > 512
        || abs(sampleBuf[620]) > 512
        || abs(sampleBuf[700]) > 512
        || abs(sampleBuf[800]) > 512
        || abs(sampleBuf[900]) > 512
        || abs(sampleBuf[999]) > 512;
}

word FX::CountPlaying()
{
    return Mix_Playing(-1);
}

void FX::SetFormat(dword samplesPerSec, word channels, word bitsPerSample)
{
    dword lastSamplesPerSec = _fxData.samplesPerSec;
    word lastChannels = _fxData.channels;
    word lastBitsPerSample = _fxData.bitsPerSample;

    _fxData.samplesPerSec = samplesPerSec ? samplesPerSec : _digitalData.pSSE->_samplesPerSec;
    _fxData.channels = channels ? channels : 1;
    _fxData.bitsPerSample = bitsPerSample ? bitsPerSample : _digitalData.pSSE->_bitsPerSample;

    if (_fxData.pBuffer[0] && (_fxData.samplesPerSec != lastSamplesPerSec ||
        _fxData.channels != lastChannels || _fxData.bitsPerSample != lastBitsPerSample))
    {
        Free();
        if (!_digitalData.file.empty())
            Load(_digitalData.file.c_str());
    }
}

MIDI::MIDI()
{
    _music = nullptr;
}

MIDI::~MIDI()
{
    Free();
}

HRESULT MIDI::Create(SSE* pSSE, char* file)
{
    _musicData.pSSE = pSSE;

    if (file)
        Load(file);

    return SSE_OK;
}

bool MIDI::StopPriority(dword flags)
{
    return false;
}

long MIDI::Release()
{
    return 0;
}

HRESULT MIDI::Play(dword dwFlags, long pan)
{
    return Mix_PlayMusic(_music, 0) < 0 ? SSE_CANNOTPLAY : SSE_OK;
}

HRESULT MIDI::Stop()
{
    return Mix_HaltMusic() < 0 ? SSE_CANNOTPLAY : SSE_OK;
}

HRESULT MIDI::Pause()
{
    Mix_PauseMusic();
    return SSE_OK;
}

HRESULT MIDI::Resume()
{
    Mix_ResumeMusic();
    return SSE_OK;
}

HRESULT MIDI::GetVolume(long* pVolume)
{
    *pVolume = Mix_VolumeMusic(-1);
    return SSE_OK;
}

HRESULT MIDI::SetVolume(long volume)
{
    Mix_VolumeMusic(volume);
    return SSE_OK;
}

HRESULT MIDI::GetPan(long* pPan)
{
    return SSE_OK;
}

HRESULT MIDI::SetPan(long pan)
{
    return SSE_OK;
}

HRESULT MIDI::Load(const char* file)
{
    if (_music)
        Free();

    _musicData.file = file;
    _music = Mix_LoadMUS(file);

    // Some version ship with ogg music as well, use it as a fall-back
    if (!_music)
        _musicData.file.replace(_musicData.file.size() - 3, 3, "ogg");
    _music = Mix_LoadMUS(file);
    return SSE_OK;
}

HRESULT MIDI::Free()
{
    Mix_FreeMusic(_music);
    return SSE_OK;
}

HRESULT MIDI::GetStatus(dword* pStatus)
{
    return SSE_OK;
}

word MIDI::CountPlaying()
{
    return Mix_PlayingMusic();
}
