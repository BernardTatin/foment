/*

Foment

-- MakeHandlePort for unix
-- combine OpenInputFile and OpenBinaryInputFilePrimitive (and for Output too)
-- replace StdioPort with BufferedPort
-- add an optional PeekBytes to BinaryPorts
-- get rid of commented sections
-- sockets

*/

#ifdef FOMENT_WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <io.h>
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#endif // FOMENT_UNIX

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "foment.hpp"
#ifdef FOMENT_BSD
#include <stdlib.h>
#else // FOMENT_BSD
#include <malloc.h>
#endif // FOMENT_BSD
#include "syncthrd.hpp"
#include "io.hpp"
#include "unicode.hpp"

#define NOT_PEEKED ((uint_t) (-1))
#define CR 13
#define LF 10

FMakeEncodedPort MakeEncodedPort = MakeLatin1Port;

// ---- Binary Ports ----

FObject MakeBinaryPort(FObject nam, FObject obj, void * ctx, FCloseInputFn cifn,
    FCloseOutputFn cofn, FFlushOutputFn fofn, FReadBytesFn rbfn, FByteReadyPFn brpfn,
    FWriteBytesFn wbfn)
{
    FAssert((cifn == 0 && rbfn == 0 && brpfn == 0) || (cifn != 0 && rbfn != 0 && brpfn != 0));
    FAssert((cofn == 0 && wbfn == 0 && fofn == 0) || (cofn != 0 && wbfn != 0 && fofn != 0));
    FAssert(cifn != 0 || cofn != 0);

    FBinaryPort * port = (FBinaryPort *) MakeObject(sizeof(FBinaryPort), BinaryPortTag);
    port->Generic.Flags = BinaryPortTag
            | (cifn != 0 ? (PORT_FLAG_INPUT | PORT_FLAG_INPUT_OPEN) : 0)
            | (cofn != 0 ? (PORT_FLAG_OUTPUT | PORT_FLAG_OUTPUT_OPEN) : 0);
    port->Generic.Name = nam;
    port->Generic.Object = obj;
    port->Generic.Context = ctx;
    port->Generic.CloseInputFn = cifn;
    port->Generic.CloseOutputFn = cofn;
    port->Generic.FlushOutputFn = fofn;
    port->ReadBytesFn = rbfn;
    port->ByteReadyPFn = brpfn;
    port->WriteBytesFn = wbfn;
    port->PeekedByte = NOT_PEEKED;
    port->Offset = 0;

    InstallGuardian(port, R.CleanupTConc);

    return(port);
}

uint_t ReadBytes(FObject port, FByte * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));
    FAssert(bl > 0);

    if (AsBinaryPort(port)->PeekedByte != NOT_PEEKED)
    {
        FAssert(AsBinaryPort(port)->PeekedByte >= 0 && AsBinaryPort(port)->PeekedByte <= 0xFF);

        *b = (FByte) AsBinaryPort(port)->PeekedByte;
        AsBinaryPort(port)->PeekedByte = NOT_PEEKED;

        if (bl == 1)
            return(1);

        return(AsBinaryPort(port)->ReadBytesFn(port, b + 1, bl - 1) + 1);
    }

    return(AsBinaryPort(port)->ReadBytesFn(port, b, bl));
}

int_t PeekByte(FObject port, FByte * b)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    if (AsBinaryPort(port)->PeekedByte != NOT_PEEKED)
    {
        FAssert(AsBinaryPort(port)->PeekedByte >= 0 && AsBinaryPort(port)->PeekedByte <= 0xFF);

        *b = (FByte) AsBinaryPort(port)->PeekedByte;
        return(1);
    }

    if (AsBinaryPort(port)->ReadBytesFn(port, b, 1) == 0)
        return(0);

    AsBinaryPort(port)->PeekedByte = *b;
    return(1);
}

int_t ByteReadyP(FObject port)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    if (AsBinaryPort(port)->PeekedByte != NOT_PEEKED)
        return(1);

    return(AsBinaryPort(port)->ByteReadyPFn(port));
}

uint_t GetOffset(FObject port)
{
    FAssert(BinaryPortP(port) && InputPortP(port));

    return(AsBinaryPort(port)->Offset);
}

void WriteBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && OutputPortOpenP(port));
    FAssert(bl > 0);

    AsBinaryPort(port)->WriteBytesFn(port, b, bl);
}

static void StdioCloseInput(FObject port)
{
    FAssert(BinaryPortP(port));

    if (OutputPortOpenP(port) == 0)
        fclose((FILE *) AsGenericPort(port)->Context);
}

static void StdioCloseOutput(FObject port)
{
    FAssert(BinaryPortP(port));

    if (InputPortOpenP(port) == 0)
        fclose((FILE *) AsGenericPort(port)->Context);
}

static void StdioFlushOutput(FObject port)
{
    FAssert(BinaryPortP(port) && OutputPortOpenP(port));

    fflush((FILE *) AsGenericPort(port)->Context);
}

static uint_t StdioReadBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));
    FAssert(AsGenericPort(port)->Context != 0);

    return(fread(b, 1, bl, (FILE *) AsGenericPort(port)->Context));
}

static int_t StdioByteReadyP(FObject port)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    return(1);
}

static void StdioWriteBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && OutputPortOpenP(port));
    FAssert(AsGenericPort(port)->Context != 0);

    fwrite(b, 1, bl, (FILE *) AsGenericPort(port)->Context);
}

static FObject MakeStdioInputPort(FObject nam, FILE * fp)
{
    return(MakeBinaryPort(nam, NoValueObject, fp, StdioCloseInput, 0, 0, StdioReadBytes,
            StdioByteReadyP, 0));
}

static FObject MakeStdioOutputPort(FObject nam, FILE * fp)
{
    return(MakeBinaryPort(nam, NoValueObject, fp, 0, StdioCloseOutput, StdioFlushOutput, 0, 0,
            StdioWriteBytes));
}

#ifdef FOMENT_WINDOWS
static void HandleCloseInput(FObject port)
{
    FAssert(BinaryPortP(port));

    if (OutputPortOpenP(port) == 0)
        CloseHandle((HANDLE) AsGenericPort(port)->Context);
}

static void HandleCloseOutput(FObject port)
{
    FAssert(BinaryPortP(port));

    if (InputPortOpenP(port) == 0)
        CloseHandle((HANDLE) AsGenericPort(port)->Context);
}

static void HandleFlushOutput(FObject port)
{
    FAssert(BinaryPortP(port) && OutputPortOpenP(port));

    FlushFileBuffers((HANDLE) AsGenericPort(port)->Context);
}

static uint_t HandleReadBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    DWORD nr;

    if (ReadFile((HANDLE) AsGenericPort(port)->Context, b, (DWORD) bl, &nr, 0) == 0)
        return(0);

    return(nr);
}

static int_t FileByteReadyP(FObject port)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    return(1);
}

static int_t PipeByteReadyP(FObject port)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));


    DWORD tba;

    return(PeekNamedPipe((HANDLE) AsGenericPort(port)->Context, 0, 0, 0, &tba, 0) == 0
            || tba > 0);
}

static void HandleWriteBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && OutputPortOpenP(port));
    FAssert(AsGenericPort(port)->Context != 0);

    DWORD nw;

    WriteFile((HANDLE) AsGenericPort(port)->Context, b, (DWORD) bl, &nw, 0);
}

static FObject MakeHandleInputPort(FObject nam, HANDLE h)
{
    return(MakeBinaryPort(nam, NoValueObject, h, HandleCloseInput, 0, 0, HandleReadBytes,
            GetFileType(h) == FILE_TYPE_PIPE ? PipeByteReadyP : FileByteReadyP, 0));
}

static FObject MakeHandleOutputPort(FObject nam, HANDLE h)
{
    return(MakeBinaryPort(nam, NoValueObject, h, 0, HandleCloseOutput, HandleFlushOutput, 0, 0,
            HandleWriteBytes));
}
#endif // FOMENT_WINDOWS

static void BvinCloseInput(FObject port)
{
    FAssert(BinaryPortP(port));

    AsGenericPort(port)->Object = NoValueObject;
}

static uint_t BvinReadBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    FObject bv = AsGenericPort(port)->Object;
    uint_t bdx = (uint_t) AsGenericPort(port)->Context;

    FAssert(BytevectorP(bv));
    FAssert(bdx <= BytevectorLength(bv));

    if (bdx == BytevectorLength(bv))
        return(0);

    if (bdx + bl > BytevectorLength(bv))
        bl = BytevectorLength(bv) - bdx;

    memcpy(b, AsBytevector(bv)->Vector + bdx, bl);
    AsGenericPort(port)->Context = (void *) (bdx + bl);

    return(bl);
}

static int_t BvinByteReadyP(FObject port)
{
    FAssert(BinaryPortP(port) && InputPortOpenP(port));

    return(1);
}

static FObject MakeBytevectorInputPort(FObject bv)
{
    FAssert(BytevectorP(bv));

    return(MakeBinaryPort(NoValueObject, bv, 0, BvinCloseInput, 0, 0, BvinReadBytes,
            BvinByteReadyP, 0));
}

static void BvoutCloseOutput(FObject port)
{
    // Nothing.
}

static void BvoutFlushOutput(FObject port)
{
    // Nothing.
}

static void BvoutWriteBytes(FObject port, void * b, uint_t bl)
{
    FAssert(BinaryPortP(port) && OutputPortOpenP(port));
    FAssert(AsGenericPort(port)->Object == EmptyListObject || PairP(AsGenericPort(port)->Object));

    FObject lst = AsGenericPort(port)->Object;

    for (uint_t bdx = 0; bdx < bl; bdx++)
        lst = MakePair(MakeFixnum(((unsigned char *) b)[bdx]), lst);

    AsGenericPort(port)->Object = lst;
}

static FObject GetOutputBytevector(FObject port)
{
    FAssert(BytevectorOutputPortP(port));
    FAssert(AsGenericPort(port)->Object == EmptyListObject || PairP(AsGenericPort(port)->Object));

    FObject lst = AsGenericPort(port)->Object;
    int_t bl = ListLength("get-output-bytevector", lst);
    FObject bv = MakeBytevector(bl);
    int_t bdx = bl;

    while (PairP(lst))
    {
        bdx -= 1;

        FAssert(bdx >= 0);
        FAssert(FixnumP(First(lst)) && AsFixnum(First(lst)) >= 0 && AsFixnum(First(lst)) <= 0xFF);

        AsBytevector(bv)->Vector[bdx] = (FByte) AsFixnum(First(lst));
        lst = Rest(lst);
    }

    FAssert(lst == EmptyListObject);

    return(bv);
}

static FObject MakeBytevectorOutputPort()
{
    FObject port = MakeBinaryPort(NoValueObject, EmptyListObject, 0, 0, BvoutCloseOutput,
            BvoutFlushOutput, 0, 0, BvoutWriteBytes);
    AsGenericPort(port)->Flags |= PORT_FLAG_BYTEVECTOR_OUTPUT;
    return(port);
}

// ---- Textual Ports ----

FObject MakeTextualPort(FObject nam, FObject obj, void * ctx, FCloseInputFn cifn,
    FCloseOutputFn cofn, FFlushOutputFn fofn, FReadChFn rcfn, FCharReadyPFn crpfn,
    FWriteStringFn wsfn)
{
    FAssert((cifn == 0 && rcfn == 0 && crpfn == 0) || (cifn != 0 && rcfn != 0 && crpfn != 0));
    FAssert((cofn == 0 && wsfn == 0 && fofn == 0) || (cofn != 0 && wsfn != 0 && fofn != 0));
    FAssert(cifn != 0 || cofn != 0);

    FTextualPort * port = (FTextualPort *) MakeObject(sizeof(FTextualPort), TextualPortTag);
    port->Generic.Flags = TextualPortTag
            | (cifn != 0 ? (PORT_FLAG_INPUT | PORT_FLAG_INPUT_OPEN) : 0)
            | (cofn != 0 ? (PORT_FLAG_OUTPUT | PORT_FLAG_OUTPUT_OPEN) : 0);
    port->Generic.Name = nam;
    port->Generic.Object = obj;
    port->Generic.Context = ctx;
    port->Generic.CloseInputFn = cifn;
    port->Generic.CloseOutputFn = cofn;
    port->Generic.FlushOutputFn = fofn;
    port->ReadChFn = rcfn;
    port->CharReadyPFn = crpfn;
    port->WriteStringFn = wsfn;
    port->PeekedChar = NOT_PEEKED;
    port->Line = 1;
    port->Column = 0;

    InstallGuardian(port, R.CleanupTConc);

    return(port);
}

uint_t ReadCh(FObject port, FCh * ch)
{
    FAssert(TextualPortP(port) && InputPortOpenP(port));

    if (AsTextualPort(port)->PeekedChar != NOT_PEEKED)
    {
        *ch = (FCh) AsTextualPort(port)->PeekedChar;
        AsTextualPort(port)->PeekedChar = NOT_PEEKED;

        return(1);
    }

    return(AsTextualPort(port)->ReadChFn(port, ch));
}

uint_t PeekCh(FObject port, FCh * ch)
{
    FAssert(TextualPortP(port) && InputPortOpenP(port));

    if (AsTextualPort(port)->PeekedChar != NOT_PEEKED)
    {
        *ch = (FCh) AsTextualPort(port)->PeekedChar;
        return(1);
    }

    if (ReadCh(port, ch) == 0)
        return(0);

    AsTextualPort(port)->PeekedChar = *ch;
    return(1);
}

int_t CharReadyP(FObject port)
{
    FAssert(TextualPortP(port) && InputPortOpenP(port));

    if (AsTextualPort(port)->PeekedChar != NOT_PEEKED)
        return(1);

    return(AsTextualPort(port)->CharReadyPFn(port));
}

static FObject ListToString(FObject lst)
{
    uint_t sl = ListLength("read-line", lst);
    FObject s = MakeString(0, sl);
    uint_t sdx = sl;

    while (sdx > 0)
    {
        sdx -= 1;
        AsString(s)->String[sdx] = AsCharacter(First(lst));
        lst = Rest(lst);
    }

    return(s);
}

FObject ReadLine(FObject port)
{
    FAssert(TextualPortP(port) && InputPortOpenP(port));

    FObject lst = EmptyListObject;
    FCh ch = 0;

    while (ReadCh(port, &ch))
    {
        if (ch == 0x0A || ch == 0x0D)
            break;

        lst = MakePair(MakeCharacter(ch), lst);
    }

    if (ch == 0x0D) // carriage return
    {
        while(PeekCh(port, &ch))
        {
            if (ch == 0x0A) // linefeed
            {
                ReadCh(port, &ch);
                break;
            }

            if (ch != 0x0D) // carriage return
                break;

            ReadCh(port, &ch);
        }
    }

    if (ch == 0 && lst == EmptyListObject)
        return(EndOfFileObject);

    return(ListToString(lst));
}

FObject ReadString(FObject port, uint_t cnt)
{
    FAssert(TextualPortP(port) && InputPortOpenP(port));

    FObject lst = EmptyListObject;
    FCh ch;

    while (cnt > 0 && ReadCh(port, &ch))
    {
        cnt -= 1;
        lst = MakePair(MakeCharacter(ch), lst);
    }

    if (lst == EmptyListObject)
        return(EndOfFileObject);

    return(ListToString(lst));
}

uint_t GetLineColumn(FObject port, uint_t * col)
{
    FAssert(TextualPortP(port) && InputPortP(port));

    if (col)
        *col = AsTextualPort(port)->Column;
    return(AsTextualPort(port)->Line);
}

void FoldcasePort(FObject port, int_t fcf)
{
    FAssert(TextualPortP(port) && InputPortP(port));

    if (fcf)
        AsGenericPort(port)->Flags |= PORT_FLAG_FOLDCASE;
    else
        AsGenericPort(port)->Flags &= ~PORT_FLAG_FOLDCASE;
}

void WantIdentifiersPort(FObject port, int_t wif)
{
    FAssert(TextualPortP(port) && InputPortP(port));

    if (wif)
        AsGenericPort(port)->Flags |= PORT_FLAG_WANT_IDENTIFIERS;
    else
        AsGenericPort(port)->Flags &= ~PORT_FLAG_WANT_IDENTIFIERS;
}

void WriteCh(FObject port, FCh ch)
{
    FAssert(TextualPortP(port) && OutputPortOpenP(port));

    AsTextualPort(port)->WriteStringFn(port, &ch, 1);
}

void WriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(TextualPortP(port) && OutputPortOpenP(port));

    AsTextualPort(port)->WriteStringFn(port, s, sl);
}

void WriteStringC(FObject port, const char * s)
{
    FAssert(TextualPortP(port) && OutputPortOpenP(port));

    while (*s)
    {
        FCh ch = *s;
        AsTextualPort(port)->WriteStringFn(port, &ch, 1);
        s += 1;
    }
}

void CloseInput(FObject port)
{
    FAssert(BinaryPortP(port) || TextualPortP(port));

    if (InputPortOpenP(port))
    {
        AsGenericPort(port)->Flags &= ~PORT_FLAG_INPUT_OPEN;

        AsGenericPort(port)->CloseInputFn(port);
    }
}

void CloseOutput(FObject port)
{
    FAssert(BinaryPortP(port) || TextualPortP(port));

    if (OutputPortOpenP(port))
    {
        AsGenericPort(port)->Flags &= ~PORT_FLAG_OUTPUT_OPEN;

        AsGenericPort(port)->CloseOutputFn(port);
    }
}

void FlushOutput(FObject port)
{
    FAssert(BinaryPortP(port) || TextualPortP(port));
    FAssert(OutputPortOpenP(port));

    AsGenericPort(port)->FlushOutputFn(port);
}

static void TranslatorCloseInput(FObject port)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    CloseInput(AsGenericPort(port)->Object);
}

static void TranslatorCloseOutput(FObject port)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    CloseOutput(AsGenericPort(port)->Object);
}

static void TranslatorFlushOutput(FObject port)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    FlushOutput(AsGenericPort(port)->Object);
}

static FObject MakeTranslatorPort(FObject port, FReadChFn rcfn, FCharReadyPFn crpfn,
    FWriteStringFn wsfn)
{
    FAssert(BinaryPortP(port));

    return(MakeTextualPort(AsGenericPort(port)->Name, port, 0,
            InputPortP(port) ? TranslatorCloseInput : 0,
            OutputPortP(port) ? TranslatorCloseOutput : 0,
            OutputPortP(port) ? TranslatorFlushOutput : 0,
            InputPortP(port) ? rcfn : 0,
            InputPortP(port) ? crpfn : 0,
            OutputPortP(port) ? wsfn : 0));
}

static uint_t Latin1ReadCh(FObject port, FCh * ch)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    unsigned char b;

    if (ReadBytes(AsGenericPort(port)->Object, &b, 1) != 1)
        return(0);

    *ch = b;
    if (b == CR)
    {
        if (PeekByte(AsGenericPort(port)->Object, &b) != 0 && b != LF)
        {
            AsTextualPort(port)->Line += 1;
            AsTextualPort(port)->Column = 0;
        }
    }
    else if (b == LF)
    {
        AsTextualPort(port)->Line += 1;
        AsTextualPort(port)->Column = 0;
    }

    return(1);
}

static int_t Latin1CharReadyP(FObject port)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    return(ByteReadyP(AsGenericPort(port)->Object));
}

static void Latin1WriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    for (uint_t sdx = 0; sdx < sl; sdx++)
    {
        unsigned char b;

        if (s[sdx] > 0xFF)
            b = '?';
        else
            b = (unsigned char) s[sdx];

        WriteBytes(AsGenericPort(port)->Object, &b, 1);
    }
}

FObject MakeLatin1Port(FObject port)
{
    return(MakeTranslatorPort(port, Latin1ReadCh, Latin1CharReadyP, Latin1WriteString));
}

static uint_t Utf8ReadCh(FObject port, FCh * ch)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    unsigned char ub[6];

    if (ReadBytes(AsGenericPort(port)->Object, ub, 1) != 1)
        return(0);

    uint_t tb = Utf8TrailingBytes[ub[0]];
    if (tb == 0)
    {
        *ch = ub[0];
        return(1);
    }

    if (ReadBytes(AsGenericPort(port)->Object, ub + 1, tb) != tb)
        return(0);

    *ch = ConvertUtf8ToCh(ub, tb + 1);
    return(1);
}

static int_t Utf8CharReadyP(FObject port)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    return(ByteReadyP(AsGenericPort(port)->Object));
}

static void Utf8WriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    FObject bv = ConvertStringToUtf8(s, sl, 0);

    FAssert(BytevectorP(bv));

    WriteBytes(AsGenericPort(port)->Object, AsBytevector(bv)->Vector, BytevectorLength(bv));
}

FObject MakeUtf8Port(FObject port)
{
    return(MakeTranslatorPort(port, Utf8ReadCh, Utf8CharReadyP, Utf8WriteString));
}

static uint_t Utf16ReadCh(FObject port, FCh * pch)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    FCh16 ch16;

    if (ReadBytes(AsGenericPort(port)->Object, (FByte *) &ch16, 2) != 2)
        return(0);

    FCh ch = ch16;

    if (ch >= Utf16HighSurrogateStart && ch <= Utf16HighSurrogateEnd)
    {
        if (ReadBytes(AsGenericPort(port)->Object, (FByte *) &ch16, 2) != 2)
            return(0);

        if (ch16 >= Utf16LowSurrogateStart && ch16 <= Utf16LowSurrogateEnd)
            ch = ((ch - Utf16HighSurrogateStart) << Utf16HalfShift)
                    + (((FCh) ch16) - Utf16LowSurrogateStart) + Utf16HalfBase;
        else
            ch = UnicodeReplacementCharacter;
    }
    else if (ch >= Utf16LowSurrogateStart && ch <= Utf16LowSurrogateEnd)
        ch = UnicodeReplacementCharacter;

    *pch = ch;
    return(1);
}

static int_t Utf16CharReadyP(FObject port)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    return(ByteReadyP(AsGenericPort(port)->Object));
}

static void Utf16WriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(BinaryPortP(AsGenericPort(port)->Object));

    FObject bv = ConvertStringToUtf16(s, sl, 0);

    FAssert(BytevectorP(bv));

    WriteBytes(AsGenericPort(port)->Object, AsBytevector(bv)->Vector, BytevectorLength(bv));
}

FObject MakeUtf16Port(FObject port)
{
    return(MakeTranslatorPort(port, Utf16ReadCh, Utf16CharReadyP, Utf16WriteString));
}

FObject OpenInputFile(FObject fn)
{
#ifdef FOMENT_WINDOWS
    FObject bv = ConvertStringToUtf16(fn);

    FAssert(BytevectorP(bv));

/*    FILE * fp = _wfopen((FCh16 *) AsBytevector(bv)->Vector, L"rb");
    if (fp == 0)
        return(NoValueObject);

    return(MakeEncodedPort(MakeStdioInputPort(fn, fp)));*/

    HANDLE h = CreateFileW((FCh16 *) AsBytevector(bv)->Vector, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        return(NoValueObject);

    return(MakeEncodedPort(MakeHandleInputPort(fn, h)));
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    FObject bv = ConvertStringToUtf8(fn);

    FAssert(BytevectorP(bv));

    FILE * fp = fopen((const char *) AsBytevector(bv)->Vector, "rb");
    if (fp == 0)
        return(NoValueObject);

    return(MakeEncodedPort(MakeStdioInputPort(fn, fp)));
#endif // FOMENT_UNIX
}

FObject OpenOutputFile(FObject fn)
{
#ifdef FOMENT_WINDOWS
    FObject bv = ConvertStringToUtf16(fn);

    FAssert(BytevectorP(bv));

/*    FILE * fp = _wfopen((FCh16 *) AsBytevector(bv)->Vector, L"wb");
    if (fp == 0)
        return(NoValueObject);

    return(MakeEncodedPort(MakeStdioOutputPort(fn, fp)));*/

    HANDLE h = CreateFileW((FCh16 *) AsBytevector(bv)->Vector, GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE)
        return(NoValueObject);

    return(MakeEncodedPort(MakeHandleOutputPort(fn, h)));
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    FObject bv = ConvertStringToUtf8(fn);

    FAssert(BytevectorP(bv));

    FILE * fp = fopen((const char *) AsBytevector(bv)->Vector, "wb");
    if (fp == 0)
        return(NoValueObject);

    return(MakeEncodedPort(MakeStdioOutputPort(fn, fp)));
#endif // FOMENT_UNIX
}

static void SinCloseInput(FObject port)
{
    FAssert(TextualPortP(port));

    AsGenericPort(port)->Object = NoValueObject;
}

static uint_t SinReadCh(FObject port, FCh * ch)
{
    FAssert(TextualPortP(port) && InputPortOpenP(port));

    FObject s = AsGenericPort(port)->Object;
    uint_t sdx = (uint_t) AsGenericPort(port)->Context;

    FAssert(StringP(s));
    FAssert(sdx <= StringLength(s));

    if (sdx == StringLength(s))
        return(0);

    *ch = AsString(s)->String[sdx];
    AsGenericPort(port)->Context = (void *) (sdx + 1);

    return(1);
}

static int_t SinCharReadyP(FObject port)
{
    return(1);
}

FObject MakeStringInputPort(FObject s)
{
    FAssert(StringP(s));

    return(MakeTextualPort(NoValueObject, s, 0, SinCloseInput, 0, 0, SinReadCh, SinCharReadyP,
            0));
}

static void SoutCloseOutput(FObject port)
{
    // Nothing.
}

static void SoutFlushOutput(FObject port)
{
    // Nothing.
}

static void SoutWriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(TextualPortP(port) && OutputPortOpenP(port));
    FAssert(AsGenericPort(port)->Object == EmptyListObject || PairP(AsGenericPort(port)->Object));

    FObject lst = AsGenericPort(port)->Object;

    for (uint_t sdx = 0; sdx < sl; sdx++)
        lst = MakePair(MakeCharacter(s[sdx]), lst);

    AsGenericPort(port)->Object = lst;
}

FObject GetOutputString(FObject port)
{
    FAssert(StringOutputPortP(port));
    FAssert(AsGenericPort(port)->Object == EmptyListObject || PairP(AsGenericPort(port)->Object));

    FObject lst = AsGenericPort(port)->Object;
    int_t sl = ListLength("get-output-string", lst);
    FObject s = MakeString(0, sl);
    int_t sdx = sl;

    while (PairP(lst))
    {
        sdx -= 1;

        FAssert(sdx >= 0);
        FAssert(CharacterP(First(lst)));

        AsString(s)->String[sdx] = AsCharacter(First(lst));
        lst = Rest(lst);
    }

    FAssert(lst == EmptyListObject);

    return(s);
}

FObject MakeStringOutputPort()
{
    FObject port = MakeTextualPort(NoValueObject, EmptyListObject, 0, 0, SoutCloseOutput,
            SoutFlushOutput, 0, 0, SoutWriteString);
    AsGenericPort(port)->Flags |= PORT_FLAG_STRING_OUTPUT;
    return(port);
}

static void CinCloseInput(FObject port)
{
    // Nothing.
}

static uint_t CinReadCh(FObject port, FCh * ch)
{
    FAssert(TextualPortP(port));

    char * s = (char *) AsGenericPort(port)->Context;

    if (*s == 0)
        return(0);

    *ch = *s;
    AsGenericPort(port)->Context = (void *) (s + 1);

    return(1);
}

static int_t CinCharReadyP(FObject port)
{
    return(1);
}

FObject MakeStringCInputPort(const char * s)
{
    return(MakeTextualPort(NoValueObject, NoValueObject, (void *) s, CinCloseInput, 0, 0,
            CinReadCh, CinCharReadyP, 0));
}

// ---- Console Input and Output ----

#define CONSOLE_INPUT_EDITLINE 0x00000001
#define CONSOLE_INPUT_ECHO     0x00000002

#define MAXIMUM_POSSIBLE 4096
#define MAXIMUM_HISTORY 64

#ifdef FOMENT_WINDOWS
typedef wchar_t FConCh;
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
typedef unsigned char FConCh;
#endif // FOMENT_UNIX

typedef struct
{
    FConCh * String;
    int_t Length;
} FHistory;

typedef struct
{
#ifdef FOMENT_WINDOWS
    HANDLE InputHandle;
    HANDLE OutputHandle;
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    int_t InputFd;
    int_t OutputFd;
#endif // FOMENT_UNIX

    uint_t Mode;

    // Raw Mode

    int_t RawUsed; // Number of available characters which have been used.
    int_t RawAvailable; // Total characters available.
    FConCh RawBuffer[4];

    // EditLine Mode

    int_t Used; // Number of available characters which have been used.
    int_t Available; // Total characters available.

    int_t Point; // Location of the point (cursor) in edit mode.
    int_t Possible; // Total characters in edit mode; when a line is entered, possible
                    // becomes available.
    int_t PreviousPossible;

    int_t EscPrefix;
    int_t EscBracketPrefix;

    int16_t StartX;
    int16_t StartY;
    int16_t Width;
    int16_t Height;

    FConCh Buffer[MAXIMUM_POSSIBLE];

    FHistory History[MAXIMUM_HISTORY];
    int_t HistoryEnd;
    int_t HistoryCurrent;
} FConsoleInput;

#define AsConsoleInput(port) ((FConsoleInput *) (AsGenericPort(port)->Context))

static void ResetConsoleInput(FConsoleInput * ci)
{
    ci->RawUsed = 0;
    ci->RawAvailable = 0;

    ci->Used = 0;
    ci->Available = 0;
    ci->Point = 0;
    ci->Possible = 0;
    ci->PreviousPossible = 0;

    ci->EscPrefix = 0;
    ci->EscBracketPrefix = 0;
}

static FConsoleInput * MakeConsoleInput()
{
    FConsoleInput * ci = (FConsoleInput *) malloc(sizeof(FConsoleInput));
    if (ci == 0)
        return(0);

    ResetConsoleInput(ci);
    ci->HistoryEnd = 0;
    ci->HistoryCurrent = 0;

    for (int_t idx = 0; idx < MAXIMUM_HISTORY; idx++)
        ci->History[idx].String = 0;

    return(ci);
}

static void ConFreeHistory(FConsoleInput * ci)
{
    for (int_t idx = 0; idx < MAXIMUM_HISTORY; idx++)
        if (ci->History[idx].String != 0)
        {
            free(ci->History[idx].String);
            ci->History[idx].String = 0;
        }
}

static void FreeConsoleInput(FConsoleInput * ci)
{
    ConFreeHistory(ci);
    free(ci);
}

static int_t NextHistory(int_t hdx)
{
    return(hdx == MAXIMUM_HISTORY - 1 ? 0 : hdx + 1);
}

static int_t PreviousHistory(int_t hdx)
{
    return(hdx == 0 ? MAXIMUM_HISTORY - 1 : hdx - 1);
}

static void ConAddHistory(FConsoleInput * ci, FConCh * b, int_t bl)
{
    FConCh * s = (FConCh *) malloc(sizeof(FConCh) * bl);
    if (s != 0)
    {
        memcpy(s, b, bl * sizeof(FConCh));

        if (ci->History[ci->HistoryEnd].String != 0)
            free(ci->History[ci->HistoryEnd].String);

        ci->History[ci->HistoryEnd].String = s;
        ci->History[ci->HistoryEnd].Length = bl;

        ci->HistoryEnd = NextHistory(ci->HistoryEnd);
    }
}

static void ConAddHistory(FConsoleInput * ci)
{
    ConAddHistory(ci, ci->Buffer, ci->Possible);
}

static void ConHistoryMove(FConsoleInput * ci, int_t hdx)
{
    if (ci->History[hdx].String != 0)
    {
        FAssert(ci->History[hdx].Length < MAXIMUM_POSSIBLE);

        memcpy(ci->Buffer, ci->History[hdx].String, ci->History[hdx].Length * sizeof(FConCh));
        ci->Possible = ci->History[hdx].Length;
        ci->Point = ci->History[hdx].Length;

        ci->HistoryCurrent = hdx;
    }
}

static void ConHistoryPrevious(FConsoleInput * ci)
{
    FAssert(ci->HistoryCurrent >= 0 && ci->HistoryCurrent < MAXIMUM_HISTORY);

    ConHistoryMove(ci, PreviousHistory(ci->HistoryCurrent));
}

static void ConHistoryNext(FConsoleInput * ci)
{
    FAssert(ci->HistoryCurrent >= 0 && ci->HistoryCurrent < MAXIMUM_HISTORY);

    ConHistoryMove(ci, NextHistory(ci->HistoryCurrent));
}

static FObject ConHistory(FConsoleInput * ci)
{
    FObject lst = EmptyListObject;
    int_t hdx = ci->HistoryEnd;

    for (;;)
    {
        if (ci->History[hdx].String != 0)
        {
#ifdef FOMENT_WINDOWS
            FAssert(sizeof(FCh16) == sizeof(FConCh));

            lst = MakePair(ConvertUtf16ToString(ci->History[hdx].String, ci->History[hdx].Length),
                    lst);
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
            FAssert(sizeof(FCh8) == sizeof(FConCh));

            lst = MakePair(ConvertUtf8ToString(ci->History[hdx].String, ci->History[hdx].Length),
                    lst);
#endif // FOMENT_UNIX
        }

        hdx = PreviousHistory(hdx);
        if (hdx == ci->HistoryEnd)
            break;
    }

    return(lst);
}

static void ConSetHistory(FConsoleInput * ci, FObject lst)
{
    ConFreeHistory(ci);
    ci->HistoryEnd = 0;

    FObject frm = lst;

    for (;;)
    {
        if (lst == EmptyListObject)
            break;

        if (PairP(lst) == 0 || StringP(First(lst)) == 0)
            RaiseExceptionC(R.Assertion, "set-console-input-history!",
                    "expected a list of strings", List(frm, lst));

        FObject bv;

#ifdef FOMENT_WINDOWS
        FAssert(sizeof(FCh16) == sizeof(FConCh));

        bv = ConvertStringToUtf16(AsString(First(lst))->String, StringLength(First(lst)), 0);
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
        FAssert(sizeof(FCh8) == sizeof(FConCh));

        bv = ConvertStringToUtf8(AsString(First(lst))->String, StringLength(First(lst)), 0);
#endif // FOMENT_UNIX

        FAssert(BytevectorP(bv));

        int_t bl = BytevectorLength(bv) / sizeof(FConCh);
        if (bl >= MAXIMUM_POSSIBLE)
            bl = MAXIMUM_POSSIBLE - 1;

        ConAddHistory(ci, (FConCh *) AsBytevector(bv)->Vector, bl);

        lst = Rest(lst);
    }

    ci->HistoryCurrent = ci->HistoryEnd;
}

static void ConSaveHistory(FConsoleInput * ci, FObject fn)
{
    try
    {
        FObject port = OpenOutputFile(fn);
        WriteSimple(port, ConHistory(ci), 0);
        CloseOutput(port);
    }
    catch (FObject obj)
    {
        ((FObject) obj);
    }
}

static void ConLoadHistory(FConsoleInput * ci, FObject fn)
{
    try
    {
        FObject port = OpenInputFile(fn);
        ConSetHistory(ci, Read(port));
        CloseInput(port);
    }
    catch (FObject obj)
    {
        ((FObject) obj);
    }
}

static void ConNotifyThrow(FConsoleInput * ci)
{
    ci->RawUsed = 0;
    ci->RawAvailable = 0;

    ci->Used = 0;
    ci->Available = 0;
    ci->Point = 0;
    ci->Possible = 0;
    ci->PreviousPossible = 0;

    ci->EscPrefix = 0;
    ci->EscBracketPrefix = 0;

    ci->HistoryCurrent = ci->HistoryEnd;

    FNotifyThrow nt = {0};

    throw nt;
}

#ifdef FOMENT_WINDOWS
static void ConCloseInput(FObject port)
{
    FAssert(InputPortP(port) && ConsolePortP(port));

    CloseHandle(AsConsoleInput(port)->InputHandle);
    CloseHandle(AsConsoleInput(port)->OutputHandle);

    FreeConsoleInput(AsConsoleInput(port));
}

static uint_t ConRawEsc(FConsoleInput * ci, FConCh ch1, FConCh ch2)
{
    ci->RawUsed = 0;
    ci->RawAvailable = 3;
    ci->RawBuffer[0] = 27; // Esc
    ci->RawBuffer[1] = ch1;
    ci->RawBuffer[2] = ch2;

    return(1);
}

static uint_t ConReadRaw(FConsoleInput * ci, int_t wif)
{
    FAssert(ci->RawAvailable == 0);

    INPUT_RECORD ir;

    for (;;)
    {
        for (;;)
        {
            DWORD ret = WaitForSingleObjectEx(ci->InputHandle, wif ? INFINITE : 0, TRUE);

            if (ret == WAIT_OBJECT_0)
                break;

            if (ret == WAIT_TIMEOUT)
            {
                FAssert(wif == 0);

                return(1);
            }

            if (ret != WAIT_IO_COMPLETION)
                return(0);

            if (GetThreadState()->NotifyFlag)
                ConNotifyThrow(ci);
        }

        DWORD ne;

        if (ReadConsoleInput(ci->InputHandle, &ir, 1, &ne) == 0)
            return(0);

        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
        {
//printf("[%d %d %x %d]\n", ir.Event.KeyEvent.wRepeatCount, ir.Event.KeyEvent.wVirtualKeyCode,
//        ir.Event.KeyEvent.dwControlKeyState, ir.Event.KeyEvent.uChar.UnicodeChar);

            if (ir.Event.KeyEvent.uChar.UnicodeChar != 0)
                break;

            if (ir.Event.KeyEvent.wVirtualKeyCode == 37) // Left Arrow
                return(ConRawEsc(ci, '[', 'D'));

            if (ir.Event.KeyEvent.wVirtualKeyCode == 38) // Up Arrow
                return(ConRawEsc(ci, '[', 'A'));

            if (ir.Event.KeyEvent.wVirtualKeyCode == 39) // Right Arrow
                return(ConRawEsc(ci, '[', 'C'));

            if (ir.Event.KeyEvent.wVirtualKeyCode == 40) // Down Arrow
                return(ConRawEsc(ci, '[', 'B'));
        }
    }

    FAssert(ir.Event.KeyEvent.uChar.UnicodeChar != 0);

    ci->RawUsed = 0;

    if ((ir.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED)
            || (ir.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED))
    {
        ci->RawAvailable = 2;
        ci->RawBuffer[0] = 27; // Esc
        ci->RawBuffer[1] = ir.Event.KeyEvent.uChar.UnicodeChar;
    }
    else if (ir.Event.KeyEvent.uChar.UnicodeChar == 13) // Carriage Return
    {
        ci->RawAvailable = 1;
        ci->RawBuffer[0] = 10; // Linefeed
    }
    else
    {
        ci->RawAvailable = 1;
        ci->RawBuffer[0] = ir.Event.KeyEvent.uChar.UnicodeChar;
    }

    return(1);
}

static void ConWriteCh(FConsoleInput * ci, FConCh ch)
{
    DWORD nc;
    WriteConsoleW(ci->OutputHandle, &ch, 1, &nc, 0);
}

static void ConGetInfo(FConsoleInput * ci)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(ci->OutputHandle, &csbi);
    ci->StartX = csbi.dwCursorPosition.X;
    ci->StartY = csbi.dwCursorPosition.Y;
    ci->Width = csbi.dwSize.X;
    ci->Height = csbi.dwSize.Y;
}

static void ConSetCursor(FConsoleInput * ci, int_t x, int_t y)
{
    COORD cp;

    cp.X = (SHORT) x;
    cp.Y = (SHORT) y;
    SetConsoleCursorPosition(ci->OutputHandle, cp);
}

static void ConWriteBuffer(FConsoleInput * ci)
{
    COORD cp;
    DWORD nc;

    cp.X = ci->StartX;
    cp.Y = ci->StartY;
    WriteConsoleOutputCharacterW(ci->OutputHandle, ci->Buffer, (DWORD) ci->Possible, cp, &nc);
}

static void ConFillExtra(FConsoleInput * ci)
{
    COORD cp;
    DWORD nc;

    cp.X = (ci->StartX + ci->Possible) % ci->Width;
    cp.Y = (SHORT) (ci->StartY + (ci->StartX + ci->Possible) / ci->Width);
    FillConsoleOutputCharacterW(ci->OutputHandle, ' ',
            (DWORD) (ci->PreviousPossible - ci->Possible), cp, &nc);
}
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
static void ConCloseInput(FObject port)
{
    FAssert(InputPortP(port) && ConsolePortP(port));

    close(AsConsoleInput(port)->InputFd);
    close(AsConsoleInput(port)->OutputFd);

    FreeConsoleInput(AsConsoleInput(port));
}

static uint_t ConReadRaw(FConsoleInput * ci, int_t wif)
{
    FAssert(ci->RawAvailable == 0);

    int_t ret;

    for (;;)
    {
        fd_set fds;
        timeval tv;

        FD_ZERO(&fds);
        FD_SET(0, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        ret = select(1, &fds, 0, 0, wif ? 0 : &tv);

        if (ret > 0)
            break;

        if (ret == 0)
        {
            FAssert(wif == 0);

            return(1);
        }

        if (GetThreadState()->NotifyFlag)
            ConNotifyThrow(ci);
    }

    ret = read(0, ci->RawBuffer, sizeof(ci->RawBuffer));

    if (ret < 1)
        return(1);

    if (ci->RawBuffer[0] == 13) // Carriage Return
        ci->RawBuffer[0] = 10; // Linefeed
    else if (ci->RawBuffer[0] == 127) // Backspace
        ci->RawBuffer[0] = 8; // ctrl-h

    ci->RawUsed = 0;
    ci->RawAvailable = ret;

    return(1);
}

static void ConWriteCh(FConsoleInput * ci, FConCh ch)
{
    write(1, &ch, sizeof(ch));
}

static void ConSetCursor(FConsoleInput * ci, int_t x, int_t y)
{
    char buf[16];

    sprintf(buf, "\x1B[%d;%dH", (int32_t) y, (int32_t) x);
    write(1, buf, strlen(buf));
}

static int_t ConGetCursor(int_t * x, int_t * y)
{
    char buf[16];
    char * str;
    int_t ret;

    strcpy(buf, "\x1B[6n");
    write(1, buf, strlen(buf));
    ret = read(0, buf, sizeof(buf));
    if (ret < 6)
        return(0);

    buf[ret] = 0;
    if (buf[0] != 27 || buf[1] != '[')
        return(0);

    str = buf + 2;
    if (strstr(str, ";") == 0)
        return(0);

    *strstr(str, ";") = 0;
    *y = atoi(str);
    str += strlen(str) + 1;
    if (strstr(str, "R") == 0)
        return(0);

    *strstr(str, "R") = 0;
    *x = atoi(str);

    return(1);
}

static void ConGetInfo(FConsoleInput * ci)
{
    int_t ret;
    int_t x;
    int_t y;

    ((int_t) ret);

    ret = ConGetCursor(&x, &y);

    FAssert(ret != 0);

    ci->StartX = x;
    ci->StartY = y;

    ConSetCursor(ci, 999, 999);
    ret = ConGetCursor(&x, &y);

    FAssert(ret != 0);

    ci->Width = x;
    ci->Height = y;

    ConSetCursor(ci, ci->StartX, ci->StartY);
}

static void ConWriteBuffer(FConsoleInput * ci)
{
    ConSetCursor(ci, ci->StartX, ci->StartY);
    write(1, ci->Buffer, ci->Possible);
}

static void ConFillExtra(FConsoleInput * ci)
{
    int_t x = (ci->StartX + ci->Possible) % ci->Width;
    int_t y = (ci->StartY + (ci->StartX + ci->Possible) / ci->Width);

    ConSetCursor(ci, x, y);

    FConCh ch = ' ';
    for (int_t idx = ci->Possible; idx < ci->PreviousPossible; idx++)
        write(1, &ch, 1);
}
#endif // FOMENT_UNIX

static uint_t ConReadRawCh(FConsoleInput * ci, FCh * ch)
{
    if (ci->RawAvailable == 0)
    {
        if (ConReadRaw(ci, 1) == 0)
            return(0);
    }

    FAssert(ci->RawAvailable > 0 && ci->RawUsed < ci->RawAvailable);

    *ch = ci->RawBuffer[ci->RawUsed];
    ci->RawUsed += 1;

    if (ci->RawUsed == ci->RawAvailable)
        ci->RawAvailable = 0;

    return(1);
}

static uint_t ConRawChReadyP(FConsoleInput * ci)
{
    if (ci->RawUsed < ci->RawAvailable)
        return(1);

    return(ConReadRaw(ci, 0) == 0 || ci->RawUsed < ci->RawAvailable);
}

static void InsertCh(FConsoleInput * ci, FConCh ch)
{
    if (ch == 10 || ci->Possible < MAXIMUM_POSSIBLE - 1)
    {
        for (int_t idx = ci->Possible; idx > ci->Point; idx--)
            ci->Buffer[idx] = ci->Buffer[idx - 1];

        ci->Buffer[ci->Point] = ch;
        ci->Point += 1;
        ci->Possible += 1;
    }
}

static void DeleteCh(FConsoleInput * ci, int_t p)
{
    FAssert(p > 0 && p <= ci->Possible);

    ci->Possible -= 1;

    if (ci->Point >= p)
        ci->Point -= 1;

    for (int_t idx = p - 1; idx < ci->Possible; idx++)
        ci->Buffer[idx] = ci->Buffer[idx + 1];
}

static void Redraw(FConsoleInput * ci)
{
    if (ci->Mode & CONSOLE_INPUT_ECHO)
    {
        while (ci->Possible + ci->StartX + 1 >= ci->Width
                && ((ci->Possible + ci->StartX) / ci->Width) + ci->StartY >= ci->Height)
        {
            ConSetCursor(ci, ci->Width, ci->Height);
            ConWriteCh(ci, '\n');

            FAssert(ci->StartY > 0);

            ci->StartY -= 1;
        }

        ConWriteBuffer(ci);

        if (ci->Possible < ci->PreviousPossible)
            ConFillExtra(ci);

        ConSetCursor(ci, (ci->StartX + ci->Point) % ci->Width,
                ci->StartY + (ci->StartX + ci->Point) / ci->Width);

        ci->PreviousPossible = ci->Possible;
    }
}

static uint_t ConEditLine(FConsoleInput * ci, int_t wif)
{
    FAssert(ci->Available == 0);

    if (ci->Possible == 0)
        ConGetInfo(ci);

    for (;;)
    {
        int_t rdf = 0;
        FCh ch;

        if (wif == 0 && ConRawChReadyP(ci) == 0)
            return(1);

        if (ConReadRawCh(ci, &ch) == 0)
            return(0);

        if (ci->EscPrefix)
        {
            FAssert(ci->EscBracketPrefix == 0);

            ci->EscPrefix = 0;

            if (ch == '[')
                ci->EscBracketPrefix = 1;
            else if (ch == 'b')
            {
                // backward-word

                if (ci->Point > 0)
                {
                    ci->Point -= 1;

                    while (IdentifierSubsequentP(ci->Buffer[ci->Point]) == 0 && ci->Point > 0)
                        ci->Point -= 1;

                    while (ci->Point > 0 && IdentifierSubsequentP(ci->Buffer[ci->Point - 1]))
                        ci->Point -= 1;

                    rdf = 1;
                }
            }
            else if (ch == 'd')
            {
                // kill-word

                while (ci->Point < ci->Possible
                        && IdentifierSubsequentP(ci->Buffer[ci->Point]) == 0)
                    DeleteCh(ci, ci->Point + 1);

                while (ci->Point < ci->Possible
                        && IdentifierSubsequentP(ci->Buffer[ci->Point]))
                    DeleteCh(ci, ci->Point + 1);

                rdf = 1;
            }
            else if (ch == 'f')
            {
                // forward-word

                if (ci->Point < ci->Possible)
                {
                    ci->Point += 1;

                    while (IdentifierSubsequentP(ci->Buffer[ci->Point]) == 0
                            && ci->Point < ci->Possible)
                        ci->Point += 1;

                    while (ci->Point < ci->Possible
                            && IdentifierSubsequentP(ci->Buffer[ci->Point]))
                        ci->Point += 1;

                    rdf = 1;
                }
            }
        }
        else if (ci->EscBracketPrefix)
        {
            FAssert(ci->EscPrefix == 0);

            ci->EscBracketPrefix = 0;

            if (ch == 'A') // Up Arrow
            {
                // previous-history

                ConHistoryPrevious(ci);
                rdf = 1;
            }
            else if (ch == 'B') // Down Arrow
            {
                // next-history

                ConHistoryNext(ci);
                rdf = 1;
            }
            else if (ch == 'C') // Right Arrow
            {
                // forward-char

                if (ci->Point < ci->Possible)
                {
                    ci->Point += 1;
                    rdf = 1;
                }
            }
            else if (ch == 'D') // Left Arrow
            {
                // backward-char

                if (ci->Point > 0)
                {
                    ci->Point -= 1;
                    rdf = 1;
                }
            }
        }
        else if (ch == 1) // ctrl-a
        {
            // beginning-of-line

            ci->Point = 0;
            rdf = 1;
        }
        else if (ch == 2) // ctrl-b
        {
            // backward-char

            if (ci->Point > 0)
            {
                ci->Point -= 1;
                rdf = 1;
            }
        }
        else if (ch == 4) // ctrl-d
        {
            // delete-char

#ifdef FOMENT_UNIX
            if (ci->Possible == 0)
            {
                FAssert(ci->Point == 0);

                ci->Buffer[ci->Possible] = 4;
                ci->Possible += 1;

                break;
            }
#endif // FOMENT_UNIX

            if (ci->Point < ci->Possible)
            {
                DeleteCh(ci, ci->Point + 1);
                rdf = 1;
            }
        }
        else if (ch == 5) // ctrl-e
        {
            // end-of-line

            ci->Point = ci->Possible;
            rdf = 1;
        }
        else if (ch == 6) // ctrl-f
        {
            // forward-char

            if (ci->Point < ci->Possible)
            {
                ci->Point += 1;
                rdf = 1;
            }
        }
        else if (ch == 8) // ctrl-h
        {
            // delete-backward-char

            if (ci->Point > 0)
            {
                DeleteCh(ci, ci->Point);
                rdf = 1;
            }
        }
        else if (ch == 10) // Linefeed
        {
            ci->Point = ci->Possible;
            Redraw(ci);

            if (ci->Possible > 0)
                ConAddHistory(ci);

            InsertCh(ci, 10); // Linefeed
            ConWriteCh(ci, 10);

            break;
        }
        else if (ch == 11) // ctrl-k
        {
            // kill-line

            ci->Possible = ci->Point;
            rdf = 1;
        }
        else if (ch == 12) // ctrl-l
        {
            // redisplay

            rdf = 1;

            ci->PreviousPossible = MAXIMUM_POSSIBLE;
        }
        else if (ch == 14) // ctrl-n
        {
            // next-history

            ConHistoryNext(ci);
            rdf = 1;
        }
        else if (ch == 16) // ctrl-p
        {
            // previous-history

            ConHistoryPrevious(ci);
            rdf = 1;
        }
#ifdef FOMENT_WINDOWS
        else if (ch == 26) // ctrl-z
        {
            if (ci->Possible == 0)
            {
                FAssert(ci->Point == 0);

                ci->Buffer[ci->Possible] = 26;
                ci->Possible += 1;

                break;
            }
        }
#endif // FOMENT_WINDOWS
        else if (ch == 27) // Esc
            ci->EscPrefix = 1;
        else if (ch == ')')
        {
            InsertCh(ci, ch);

            int_t pt = ci->Point;
            int_t cnt = 0;

            while (ci->Point > 0)
            {
                ci->Point -= 1;

                if (ci->Buffer[ci->Point] == '(')
                {
                    cnt -= 1;

                    if (cnt == 0)
                    {
                        Redraw(ci);

                        time_t t = time(0) + 2;

                        while (t > time(0))
                            if (ConRawChReadyP(ci))
                                break;
                    }
                }
                else if (ci->Buffer[ci->Point] == ')')
                    cnt += 1;
            }

            ci->Point = pt;

            rdf = 1;
        }
        else if (ch >= ' ')
        {
            InsertCh(ci, ch);
            rdf = 1;
        }

        if (rdf)
            Redraw(ci);
    }

    ci->RawUsed = 0;
    ci->RawAvailable = 0;

    ci->Used = 0;
    ci->Available = ci->Possible;
    ci->Point = 0;
    ci->Possible = 0;
    ci->PreviousPossible = 0;

    ci->EscPrefix = 0;
    ci->EscBracketPrefix = 0;

    ci->HistoryCurrent = ci->HistoryEnd;

    return(1);
}

static uint_t ConReadCh(FObject port, FCh * ch)
{
    FAssert(ConsolePortP(port) && InputPortOpenP(port));

    FConsoleInput * ci = AsConsoleInput(port);

    if (ci->Mode & CONSOLE_INPUT_EDITLINE)
    {
        if (ci->Available == 0)
        {
            if (ConEditLine(ci, 1) == 0)
                return(0);
        }

        FAssert(ci->Available > 0 && ci->Used < ci->Available);

#ifdef FOMENT_WINDOWS
        if (ci->Buffer[ci->Used] == 26) // ctrl-z
            return(0);
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
        if (ci->Buffer[ci->Used] == 4) // ctrl-d
            return(0);
#endif // FOMENT_UNIX

        *ch = ci->Buffer[ci->Used];
        ci->Used += 1;

        if (ci->Used == ci->Available)
            ci->Available = 0;

        return(1);
    }

    if (ConReadRawCh(ci, ch) == 0)
        return(0);

#ifdef FOMENT_WINDOWS
    if (*ch == 26) // ctrl-z
        return(0);
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    if (*ch == 4) // ctrl-d
        return(0);
#endif // FOMENT_UNIX

    if (ci->Mode & CONSOLE_INPUT_ECHO)
        ConWriteCh(ci, *ch);

    return(1);
}

static int_t ConCharReadyP(FObject port)
{
    FAssert(ConsolePortP(port) && InputPortOpenP(port));

    FConsoleInput * ci = AsConsoleInput(port);

    if (ci->Mode & CONSOLE_INPUT_EDITLINE)
    {
        if (ci->Used < ci->Available)
            return(1);

        return(ConEditLine(ci, 0) == 0 || ci->Used < ci->Available);
    }

    return(ConRawChReadyP(ci));
}

#ifdef FOMENT_WINDOWS
static FObject MakeConsoleInputPort(FObject nam, HANDLE hin, HANDLE hout)
{
    FConsoleInput * ci = MakeConsoleInput();
    if (ci == 0)
        return(NoValueObject);

    ci->InputHandle = hin;
    ci->OutputHandle = hout;

    FObject port = MakeTextualPort(nam, NoValueObject, ci, ConCloseInput, 0, 0, ConReadCh,
            ConCharReadyP, 0);
    AsGenericPort(port)->Flags |= PORT_FLAG_CONSOLE;
    AsConsoleInput(port)->Mode = CONSOLE_INPUT_ECHO;

    return(port);
}

static void ConCloseOutput(FObject port)
{
    FAssert(TextualPortP(port));

    CloseHandle((HANDLE) AsGenericPort(port)->Context);
}

static void ConFlushOutput(FObject port)
{
    // Nothing.
}

static void ConWriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(TextualPortP(port) && OutputPortOpenP(port));

    FObject bv = ConvertStringToUtf16(s, sl, 0);
    DWORD nc;

    FAssert(BytevectorP(bv));

    WriteConsoleW((HANDLE) AsGenericPort(port)->Context,
            (FCh16 *) AsBytevector(bv)->Vector, (DWORD) BytevectorLength(bv) / sizeof(FCh16),
            &nc, 0);
}

static FObject MakeConsoleOutputPort(FObject nam, HANDLE h)
{
    FObject port = MakeTextualPort(nam, NoValueObject, h, 0, ConCloseOutput, ConFlushOutput, 0,
            0, ConWriteString);
    AsGenericPort(port)->Flags |= PORT_FLAG_CONSOLE;

    return(port);
}
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
static FObject MakeConsoleInputPort(FObject nam, int_t ifd, int_t ofd)
{
    FConsoleInput * ci = MakeConsoleInput();
    if (ci == 0)
        return(NoValueObject);

    ci->InputFd = ifd;
    ci->OutputFd = ofd;

    FObject port = MakeTextualPort(nam, NoValueObject, ci, ConCloseInput, 0, 0, ConReadCh,
            ConCharReadyP, 0);
    AsGenericPort(port)->Flags |= PORT_FLAG_CONSOLE;
    AsConsoleInput(port)->Mode = CONSOLE_INPUT_ECHO;

    return(port);
}

static void ConCloseOutput(FObject port)
{
    FAssert(TextualPortP(port));

    close((int_t) AsGenericPort(port)->Context);
}

static void ConFlushOutput(FObject port)
{
    // Nothing.
}

static void ConWriteString(FObject port, FCh * s, uint_t sl)
{
    FAssert(TextualPortP(port) && OutputPortOpenP(port));

    FObject bv = ConvertStringToUtf8(s, sl, 0);

    FAssert(BytevectorP(bv));

    write((int_t) AsGenericPort(port)->Context, AsBytevector(bv)->Vector,
            BytevectorLength(bv));
}

static FObject MakeConsoleOutputPort(FObject nam, int_t ofd)
{
    FObject port = MakeTextualPort(nam, NoValueObject, (void *) ofd, 0, ConCloseOutput,
            ConFlushOutput, 0, 0, ConWriteString);
    AsGenericPort(port)->Flags |= PORT_FLAG_CONSOLE;

    return(port);
}
#endif // FOMENT_UNIX

// ---- System interface ----

Define("file-exists?", FileExistsPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("file-exists?", argc);
    StringArgCheck("file-exists?", argv[0]);

#ifdef FOMENT_WINDOWS
    FObject bv = ConvertStringToUtf16(argv[0]);

    FAssert(BytevectorP(bv));

    return(_waccess((FCh16 *) AsBytevector(bv)->Vector, 0) == 0 ? TrueObject : FalseObject);
#endif // FOMENT_WINDOWS
#ifdef FOMENT_UNIX
    FObject bv = ConvertStringToUtf8(argv[0]);

    FAssert(BytevectorP(bv));

    return(access((const char *) AsBytevector(bv)->Vector, 0) == 0 ? TrueObject : FalseObject);
#endif // FOMENT_UNIX
}

Define("delete-file", DeleteFilePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("delete-file", argc);
    StringArgCheck("delete-file", argv[0]);

#ifdef FOMENT_WINDOWS
    FObject bv = ConvertStringToUtf16(argv[0]);

    FAssert(BytevectorP(bv));

    if (_wremove((FCh16 *) AsBytevector(bv)->Vector) != 0)
#endif // FOMENT_WINDOWS
#ifdef FOMENT_UNIX
    FObject bv = ConvertStringToUtf8(argv[0]);

    FAssert(BytevectorP(bv));

    if (remove((const char *) AsBytevector(bv)->Vector) != 0)
#endif // FOMENT_UNIX
        RaiseExceptionC(R.Assertion, "delete-file", "unable to delete file", List(argv[0]));

    return(NoValueObject);
}

// ---- Input and output ----

Define("input-port?", InputPortPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("input-port?", argc);

    return(InputPortP(argv[0]) ? TrueObject : FalseObject);
}

Define("output-port?", OutputPortPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("output-port?", argc);

    return(OutputPortP(argv[0]) ? TrueObject : FalseObject);
}

Define("textual-port?", TextualPortPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("textual-port?", argc);

    return(TextualPortP(argv[0]) ? TrueObject : FalseObject);
}

Define("binary-port?", BinaryPortPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("binary-port?", argc);

    return(BinaryPortP(argv[0]) ? TrueObject : FalseObject);
}

Define("port?", PortPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("port?", argc);

    return((TextualPortP(argv[0]) || BinaryPortP(argv[0])) ? TrueObject : FalseObject);
}

Define("input-port-open?", InputPortOpenPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("input-port-open?", argc);
    PortArgCheck("input-port-open?", argv[0]);

    return(InputPortOpenP(argv[0]) ? TrueObject : FalseObject);
}

Define("output-port-open?", OutputPortOpenPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("output-port-open?", argc);
    PortArgCheck("output-port-open?", argv[0]);

    return(OutputPortOpenP(argv[0]) ? TrueObject : FalseObject);
}

Define("open-binary-input-file", OpenBinaryInputFilePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("open-binary-input-file", argc);
    StringArgCheck("open-binary-input-file", argv[0]);

#ifdef FOMENT_WINDOWS
    FObject bv = ConvertStringToUtf16(argv[0]);

    FAssert(BytevectorP(bv));

/*    FILE * fp = _wfopen((FCh16 *) AsBytevector(bv)->Vector, L"rb");
    if (fp == 0)
        RaiseExceptionC(R.Assertion, "open-binary-input-file",
                "unable to open file for input", List(argv[0]));

    return(MakeStdioInputPort(argv[0], fp));*/

    HANDLE h = CreateFileW((FCh16 *) AsBytevector(bv)->Vector, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        RaiseExceptionC(R.Assertion, "open-binary-input-file",
                "unable to open file for input", List(argv[0]));

    return(MakeHandleInputPort(argv[0], h));
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    FObject bv = ConvertStringToUtf8(argv[0]);

    FAssert(BytevectorP(bv));

    FILE * fp = fopen((const char *) AsBytevector(bv)->Vector, "rb");
    if (fp == 0)
        RaiseExceptionC(R.Assertion, "open-binary-input-file",
                "unable to open file for input", List(argv[0]));

    return(MakeStdioInputPort(argv[0], fp));
#endif // FOMENT_UNIX
}

Define("open-binary-output-file", OpenBinaryOutputFilePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("open-binary-output-file", argc);
    StringArgCheck("open-binary-output-file", argv[0]);

#ifdef FOMENT_WINDOWS
    FObject bv = ConvertStringToUtf16(argv[0]);

    FAssert(BytevectorP(bv));

/*    FILE * fp = _wfopen((FCh16 *) AsBytevector(bv)->Vector, L"wb");
    if (fp == 0)
        RaiseExceptionC(R.Assertion, "open-binary-output-file",
                "unable to open file for output", List(argv[0]));

    return(MakeStdioOutputPort(argv[0], fp));*/

    HANDLE h = CreateFileW((FCh16 *) AsBytevector(bv)->Vector, GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE)
        RaiseExceptionC(R.Assertion, "open-binary-output-file",
                "unable to open file for output", List(argv[0]));

    return(MakeHandleOutputPort(argv[0], h));
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    FObject bv = ConvertStringToUtf8(argv[0]);

    FAssert(BytevectorP(bv));

    FILE * fp = fopen((const char *) AsBytevector(bv)->Vector, "wb");
    if (fp == 0)
        RaiseExceptionC(R.Assertion, "open-binary-output-file",
                "unable to open file for output", List(argv[0]));

    return(MakeStdioOutputPort(argv[0], fp));
#endif // FOMENT_UNIX
}

Define("close-port", ClosePortPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("close-port", argc);
    PortArgCheck("close-port", argv[0]);

    CloseInput(argv[0]);
    CloseOutput(argv[0]);

    return(NoValueObject);
}

Define("close-input-port", CloseInputPortPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("close-input-port", argc);
    InputPortArgCheck("close-input-port", argv[0]);

    CloseInput(argv[0]);

    return(NoValueObject);
}

Define("close-output-port", CloseOutputPortPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("close-output-port", argc);
    OutputPortArgCheck("close-output-port", argv[0]);

    CloseOutput(argv[0]);

    return(NoValueObject);
}

Define("open-input-string", OpenInputStringPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("open-input-string", argc);
    StringArgCheck("open-input-string", argv[0]);

    return(MakeStringInputPort(argv[0]));
}

Define("open-output-string", OpenOutputStringPrimitive)(int_t argc, FObject argv[])
{
    ZeroArgsCheck("open-output-string", argc);

    return(MakeStringOutputPort());
}

Define("get-output-string", GetOutputStringPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("get-output-string", argc);
    StringOutputPortArgCheck("get-output-string", argv[0]);

    return(GetOutputString(argv[0]));
}

Define("open-input-bytevector", OpenInputBytevectorPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("open-input-bytevector", argc);
    BytevectorArgCheck("open-input-bytevector", argv[0]);

    return(MakeBytevectorInputPort(argv[0]));
}

Define("open-output-bytevector", OpenOutputBytevectorPrimitive)(int_t argc, FObject argv[])
{
    ZeroArgsCheck("open-output-bytevector", argc);

    return(MakeBytevectorOutputPort());
}

Define("get-output-bytevector", GetOutputBytevectorPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("get-output-bytevector", argc);
    BytevectorOutputPortArgCheck("get-output-bytevector", argv[0]);

    return(GetOutputBytevector(argv[0]));
}

Define("make-latin1-port", MakeLatin1PortPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("make-latin1-port", argc);
    BinaryPortArgCheck("make-latin1-port", argv[0]);

    return(MakeLatin1Port(argv[0]));
}

Define("make-utf8-port", MakeUtf8PortPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("make-utf8-port", argc);
    BinaryPortArgCheck("make-utf8-port", argv[0]);

    return(MakeUtf8Port(argv[0]));
}

Define("make-utf16-port", MakeUtf16PortPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("make-utf16-port", argc);
    BinaryPortArgCheck("make-utf16-port", argv[0]);

    return(MakeUtf16Port(argv[0]));
}

Define("want-identifiers", WantIdentifiersPrimitive)(int_t argc, FObject argv[])
{
    TwoArgsCheck("want-identifiers", argc);
    TextualInputPortArgCheck("want-identifiers", argv[0]);

    WantIdentifiersPort(argv[0], argv[1] == FalseObject ? 0 : 1);
    return(NoValueObject);
}

Define("console-port?", ConsolePortPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("console-port?", argc);

    return(ConsolePortP(argv[0]) ? TrueObject : FalseObject);
}

Define("set-console-input-editline!", SetConsoleInputEditlinePrimitive)(int_t argc, FObject argv[])
{
    TwoArgsCheck("set-console-input-editline!", argc);
    ConsoleInputPortArgCheck("set-console-input-editline!", argv[0]);
    BooleanArgCheck("set-console-input-editline!", argv[1]);

    if (argv[1] == TrueObject)
        AsConsoleInput(argv[0])->Mode |= CONSOLE_INPUT_EDITLINE;
    else
        AsConsoleInput(argv[0])->Mode &= ~CONSOLE_INPUT_EDITLINE;

    ResetConsoleInput(AsConsoleInput(argv[0]));

    return(NoValueObject);
}

Define("set-console-input-echo!", SetConsoleInputEchoPrimitive)(int_t argc, FObject argv[])
{
    TwoArgsCheck("set-console-input-echo!", argc);
    ConsoleInputPortArgCheck("set-console-input-echo!", argv[0]);
    BooleanArgCheck("set-console-input-echo!", argv[1]);

    if (argv[1] == TrueObject)
        AsConsoleInput(argv[0])->Mode |= CONSOLE_INPUT_ECHO;
    else
        AsConsoleInput(argv[0])->Mode &= ~CONSOLE_INPUT_ECHO;

    return(NoValueObject);
}

Define("%save-history", SaveHistoryPrimitive)(int_t argc, FObject argv[])
{
    FMustBe(argc == 2);
    FMustBe(ConsolePortP(argv[0]) && InputPortOpenP(argv[0]));
    FMustBe(StringP(argv[1]));

    ConSaveHistory(AsConsoleInput(argv[0]), argv[1]);
    return(NoValueObject);
}

Define("%load-history", LoadHistoryPrimitive)(int_t argc, FObject argv[])
{
    FMustBe(argc == 2);
    FMustBe(ConsolePortP(argv[0]) && InputPortOpenP(argv[0]));
    FMustBe(StringP(argv[1]));

    ConLoadHistory(AsConsoleInput(argv[0]), argv[1]);
    return(NoValueObject);
}

static FPrimitive * Primitives[] =
{
    &FileExistsPPrimitive,
    &DeleteFilePrimitive,
    &InputPortPPrimitive,
    &OutputPortPPrimitive,
    &TextualPortPPrimitive,
    &BinaryPortPPrimitive,
    &PortPPrimitive,
    &InputPortOpenPPrimitive,
    &OutputPortOpenPPrimitive,
    &OpenBinaryInputFilePrimitive,
    &OpenBinaryOutputFilePrimitive,
    &ClosePortPrimitive,
    &CloseInputPortPrimitive,
    &CloseOutputPortPrimitive,
    &OpenInputStringPrimitive,
    &OpenOutputStringPrimitive,
    &GetOutputStringPrimitive,
    &OpenInputBytevectorPrimitive,
    &OpenOutputBytevectorPrimitive,
    &GetOutputBytevectorPrimitive,
    &MakeLatin1PortPrimitive,
    &MakeUtf8PortPrimitive,
    &MakeUtf16PortPrimitive,
    &WantIdentifiersPrimitive,
    &ConsolePortPPrimitive,
    &SetConsoleInputEditlinePrimitive,
    &SetConsoleInputEchoPrimitive,
    &SaveHistoryPrimitive,
    &LoadHistoryPrimitive
};

#ifdef FOMENT_UNIX
static struct termios OriginalTios;

static void FomentAtExit(void)
{
    tcsetattr(0, TCSANOW, &OriginalTios);
}

static int_t SetupConsole()
{
    struct termios tios;

    tcgetattr(0, &OriginalTios);
    atexit(FomentAtExit);

    tcgetattr(0, &tios);
    tios.c_iflag = BRKINT;
    tios.c_lflag = ISIG;
    tios.c_cc[VMIN] = 1;
    tios.c_cc[VTIME] = 1;
    tcsetattr(0, TCSANOW, &tios);
    
    int_t x;
    int_t y;
    
    if (ConGetCursor(&x, &y) == 0)
        return(0);

    return(1);
}
#endif // FOMENT_UNIX

void SetupIO()
{
#ifdef FOMENT_WINDOWS
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE herr = GetStdHandle(STD_ERROR_HANDLE);

    if (hin != INVALID_HANDLE_VALUE && hout != INVALID_HANDLE_VALUE)
    {
        DWORD imd, omd;

        if (GetConsoleMode(hin, &imd) != 0 && GetConsoleMode(hout, &omd) != 0)
        {
            SetConsoleMode(hin, ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT);

            R.StandardInput = MakeConsoleInputPort(MakeStringC("console-input"), hin, hout);
            R.StandardOutput = MakeConsoleOutputPort(MakeStringC("console-output"), hout);
        }
        else
        {
            R.StandardInput = MakeLatin1Port(MakeHandleInputPort(MakeStringC("standard-input"),
                    hin));
            R.StandardOutput = MakeLatin1Port(MakeHandleOutputPort(MakeStringC("standard-output"),
                    hout));
        }
    }

    if (herr != INVALID_HANDLE_VALUE)
    {
        DWORD emd;

        if (GetConsoleMode(herr, &emd) != 0)
            R.StandardError = MakeConsoleOutputPort(MakeStringC("console-output"), herr);
        else
            R.StandardError = MakeLatin1Port(MakeHandleOutputPort(MakeStringC("standard-error"),
                    herr));
    }
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
    if (isatty(0) && isatty(1) && SetupConsole())
    {
        R.StandardInput = MakeConsoleInputPort(MakeStringC("console-input"), 0, 1);
        R.StandardOutput = MakeConsoleOutputPort(MakeStringC("console-output"), 1);
    }
    else
    {
        R.StandardInput = MakeUtf8Port(MakeStdioInputPort(MakeStringC("standard-input"), stdin));
        R.StandardOutput = MakeUtf8Port(MakeStdioOutputPort(MakeStringC("standard-output"),
                stdout));
    }

    R.StandardError = MakeUtf8Port(MakeStdioOutputPort(MakeStringC("standard-error"), stderr));
#endif // FOMENT_UNIX

    R.QuoteSymbol = StringCToSymbol("quote");
    R.QuasiquoteSymbol = StringCToSymbol("quasiquote");
    R.UnquoteSymbol = StringCToSymbol("unquote");
    R.UnquoteSplicingSymbol = StringCToSymbol("unquote-splicing");

    for (uint_t idx = 0; idx < sizeof(Primitives) / sizeof(FPrimitive *); idx++)
        DefinePrimitive(R.Bedrock, R.BedrockLibrary, Primitives[idx]);

    SetupWrite();
    SetupRead();
}
