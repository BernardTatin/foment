/*

Foment

*/

#include <stdio.h>
#include <string.h>
#include "foment.hpp"

#ifdef FOMENT_WINDOWS
#define StringLengthS(s) wcslen(s)
#define StringCompareS(s1, s2) wcscmp(s1, L ## s2)
#endif // FOMENT_WINDOWS

#ifdef FOMENT_UNIX
#define StringLengthS(s) strlen(s)
#define StringCompareS(s1, s2) strcmp(s1, s2)
#endif // FOMENT_UNIX

static int Usage()
{
    printf(
        "compile and run the program in FILE:\n"
        "    foment [OPTION]... FILE [ARG]...\n"
        "    -A DIR            append a library search directory\n"
        "    -I DIR            prepend a library search directory\n"
        "interactive session (repl):\n"
        "    foment [OPTION]... [FLAG]... [ARG]...\n\n"
        "    -i                interactive session\n"
        "    -e EXPR           evaluate an expression\n"
        "    -p EXPR           evaluate and print an expression\n"
        "    -l FILE           load FILE\n"
        );

    return(-1);
}

static int MissingArgument(FChS * arg)
{
#ifdef FOMENT_WINDOWS
    printf("error: expected an argument following %S\n", arg);
#endif // FOMENT_WINDOWS
#ifdef FOMENT_UNIX
    printf("error: expected an argument following %s\n", arg);
#endif // FOMENT_UNIX
    return(Usage());
}

static FObject MakeInvocation(int argc, FChS * argv[])
{
    uint_t sl = -1;

    for (int adx = 0; adx < argc; adx++)
        sl += StringLengthS(argv[adx]) + 1;

    FObject s = MakeString(0, sl);
    uint_t sdx = 0;

    for (int adx = 0; adx < argc; adx++)
    {
        sl = StringLengthS(argv[adx]);
        for (uint_t idx = 0; idx < sl; idx++)
        {
            AsString(s)->String[sdx] = argv[adx][idx];
            sdx += 1;
        }

        if (adx + 1 < argc)
        {
            AsString(s)->String[sdx] = ' ';
            sdx += 1;
        }
    }

    return(s);
}

static int ProgramMode(int adx, int argc, FChS * argv[])
{
    FAssert(adx < argc);

    FChS * s = argv[adx];
    FChS * pth = 0;

    while (*s)
    {
        if (*s == PathCh)
            pth = s;

        s += 1;
    }

    if (pth != 0)
        R.LibraryPath = MakePair(MakeStringS(argv[adx], pth - argv[adx]), R.LibraryPath);

    FObject nam = MakeStringS(argv[adx]);

    adx += 1;
    R.CommandLine = MakePair(MakeInvocation(adx, argv), MakeCommandLine(argc - adx, argv + adx));

    FObject port = OpenInputFile(nam);
    if (TextualPortP(port) == 0)
    {
#ifdef FOMENT_WINDOWS
        printf("error: unable to open program: %S\n", argv[adx - 1]);
#endif // FOMENT_WINDOWS
#ifdef FOMENT_UNIX
        printf("error: unable to open program: %s\n", argv[adx - 1]);
#endif // FOMENT_UNIX
        return(Usage());
    }

    FObject proc = CompileProgram(nam, port);

    ExecuteThunk(proc);
    return(0);
}

#ifdef FOMENT_WINDOWS
int wmain(int argc, FChS * argv[])
#endif // FOMENT_WINDOWS
#ifdef FOMENT_UNIX
int main(int argc, char * argv[])
#endif // FOMENT_UNIX
{
#ifdef FOMENT_DEBUG
    printf("Foment (Debug) Scheme %s\n", FOMENT_VERSION);
#else // FOMENT_DEBUG
    printf("Foment Scheme %s\n", FOMENT_VERSION);
#endif // FOMENT_DEBUG

    int adx = 1;
    while (adx < argc)
    {
        if (StringCompareS(argv[adx], "-no-inline-procedures") == 0)
            InlineProcedures = 0;
        else if (StringCompareS(argv[adx], "-no-inline-imports") == 0)
            InlineImports = 0;

        adx += 1;
    }

    FThreadState ts;

    try
    {
        SetupFoment(&ts, argc, argv);
    }
    catch (FObject obj)
    {
        printf("Unexpected exception: SetupFoment: %p\n", obj);
        Write(R.StandardOutput, obj, 0);
        return(1);
    }

    FAssert(argc >= 1);

    try
    {
        int adx = 1;
        while (adx < argc)
        {
            if (StringCompareS(argv[adx], "-A") == 0)
            {
                adx += 1;

                if (adx == argc)
                    return(MissingArgument(argv[adx - 1]));

                FObject lp = R.LibraryPath;

                for (;;)
                {
                    FAssert(PairP(lp));

                    if (Rest(lp) == EmptyListObject)
                        break;
                    lp = Rest(lp);
                }

//                AsPair(lp)->Rest = MakePair(MakeStringS(argv[adx]), EmptyListObject);
                SetRest(lp, MakePair(MakeStringS(argv[adx]), EmptyListObject));

                adx += 1;
            }
            else if (StringCompareS(argv[adx], "-I") == 0)
            {
                adx += 1;

                if (adx == argc)
                    return(MissingArgument(argv[adx - 1]));

                R.LibraryPath = MakePair(MakeStringS(argv[adx]), R.LibraryPath);

                adx += 1;
            }
            else if (StringCompareS(argv[adx], "-latin1") == 0)
            {
                MakeEncodedPort = MakeLatin1Port;

                adx += 1;
            }
            else if (StringCompareS(argv[adx], "-utf8") == 0)
            {
                MakeEncodedPort = MakeUtf8Port;

                adx += 1;
            }
            else if (StringCompareS(argv[adx], "-utf16") == 0)
            {
                MakeEncodedPort = MakeUtf16Port;

                adx += 1;
            }
            else if (StringCompareS(argv[adx], "-no-inline-procedures") == 0
                    || StringCompareS(argv[adx], "-no-inline-imports") == 0)
                adx += 1;
            else if (argv[adx][0] != '-')
                return(ProgramMode(adx, argc, argv));
            else
                break;
        }

        R.CommandLine = MakePair(MakeInvocation(adx, argv),
                MakeCommandLine(argc - adx, argv + adx));

        ExecuteThunk(R.InteractiveThunk);
        return(0);
//        return(RunRepl(GetInteractionEnv()));
    }
    catch (FObject obj)
    {
        if (ExceptionP(obj) == 0)
            WriteStringC(R.StandardOutput, "exception: ");
        Write(R.StandardOutput, obj, 0);
        WriteCh(R.StandardOutput, '\n');

        return(-1);
    }
}
