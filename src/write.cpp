/*

Foment

*/

#ifdef FOMENT_WINDOWS
#include <windows.h>
#endif // FOMENT_WINDOWS
#ifdef FOMENT_UNIX
#include <pthread.h>
#endif // FOMENT_UNIX
#include "foment.hpp"
#include "syncthrd.hpp"
#include "io.hpp"
#include "compile.hpp"

// Write

typedef void (*FWriteFn)(FObject port, FObject obj, int_t df, void * wfn, void * ctx);
void WriteGeneric(FObject port, FObject obj, int_t df, FWriteFn wfn, void * ctx);

typedef struct
{
    FObject HashMap;
    int_t Label;
} FWriteSharedCtx;

#define ToWriteSharedCtx(ctx) ((FWriteSharedCtx *) (ctx))

inline int_t SharedObjectP(FObject obj)
{
    return(PairP(obj) || BoxP(obj) || VectorP(obj) || ProcedureP(obj) || GenericRecordP(obj));
}

uint_t FindSharedObjects(FObject hmap, FObject obj, uint_t cnt, int_t cof)
{
    FAssert(HashMapP(hmap));

Again:

    if (SharedObjectP(obj))
    {
        FObject val = EqHashMapRef(hmap, obj, MakeFixnum(0));

        FAssert(FixnumP(val));

        EqHashMapSet(hmap, obj, MakeFixnum(AsFixnum(val) + 1));

        if (AsFixnum(val) == 0)
        {
            if (PairP(obj))
            {
                cnt = FindSharedObjects(hmap, First(obj), cnt, cof);
                if (cof != 0)
                    cnt = FindSharedObjects(hmap, Rest(obj), cnt, cof);
                else
                {
                    obj = Rest(obj);
                    goto Again;
                }
            }
            else if (BoxP(obj))
                cnt = FindSharedObjects(hmap, Unbox(obj), cnt, cof);
            else if (VectorP(obj))
            {
                for (uint_t idx = 0; idx < VectorLength(obj); idx++)
                    cnt = FindSharedObjects(hmap, AsVector(obj)->Vector[idx], cnt, cof);
            }
            else if (ProcedureP(obj))
                cnt = FindSharedObjects(hmap, AsProcedure(obj)->Code, cnt, cof);
            else
            {
                FAssert(GenericRecordP(obj));

                for (uint_t fdx = 0; fdx < RecordNumFields(obj); fdx++)
                    cnt = FindSharedObjects(hmap, AsGenericRecord(obj)->Fields[fdx], cnt, cof);
            }

            if (cof)
            {
                val = EqHashMapRef(hmap, obj, MakeFixnum(0));
                FAssert(FixnumP(val));
                FAssert(AsFixnum(val) > 0);

                if (AsFixnum(val) == 1)
                    EqHashMapDelete(hmap, obj);
            }
        }
        else
            return(cnt + 1);
    }

    return(cnt);
}

void WriteSharedObject(FObject port, FObject obj, int_t df, FWriteFn wfn, void * ctx)
{
    if (SharedObjectP(obj))
    {
        FObject val = EqHashMapRef(ToWriteSharedCtx(ctx)->HashMap, obj, MakeFixnum(1));

        FAssert(FixnumP(val));

        if (AsFixnum(val) > 0)
        {
            if (AsFixnum(val) > 1)
            {
                ToWriteSharedCtx(ctx)->Label -= 1;
                EqHashMapSet(ToWriteSharedCtx(ctx)->HashMap, obj,
                        MakeFixnum(ToWriteSharedCtx(ctx)->Label));

                WriteCh(port, '#');
                FCh s[8];
                int_t sl = FixnumAsString(- ToWriteSharedCtx(ctx)->Label, s, 10);
                WriteString(port, s, sl);
                WriteCh(port, '=');
            }

            if (PairP(obj))
            {
                WriteCh(port, '(');
                for (;;)
                {
                    wfn(port, First(obj), df, (void *) wfn, ctx);
                    if (PairP(Rest(obj)) && EqHashMapRef(ToWriteSharedCtx(ctx)->HashMap,
                            Rest(obj), MakeFixnum(1)) == MakeFixnum(1))
                    {
                        WriteCh(port, ' ');
                        obj = Rest(obj);
                    }
                    else if (Rest(obj) == EmptyListObject)
                    {
                        WriteCh(port, ')');
                        break;
                    }
                    else
                    {
                        WriteStringC(port, " . ");
                        wfn(port, Rest(obj), df, (void *) wfn, ctx);
                        WriteCh(port, ')');
                        break;
                    }
                }
            }
            else
                WriteGeneric(port, obj, df, wfn, ctx);
        }
        else
        {
            FAssert(FixnumP(val) && AsFixnum(val) < 1);

            WriteCh(port, '#');
            FCh s[8];
            int_t sl = FixnumAsString(- AsFixnum(val), s, 10);
            WriteString(port, s, sl);
            WriteCh(port, '#');
        }
    }
    else
        WriteGeneric(port, obj, df, wfn, ctx);
}

void Write(FObject port, FObject obj, int_t df)
{
    FObject hmap = MakeEqHashMap();

    if (SharedObjectP(obj))
    {
        if (FindSharedObjects(hmap, obj, 0, 1) == 0)
            WriteGeneric(port, obj, df, (FWriteFn) WriteGeneric, 0);
        else
        {
            FWriteSharedCtx ctx;
            ctx.HashMap = hmap;
            ctx.Label = 1;
            WriteSharedObject(port, obj, df, (FWriteFn) WriteSharedObject, &ctx);
        }
    }
    else
        WriteGeneric(port, obj, df, (FWriteFn) WriteGeneric, 0);
}

void WriteShared(FObject port, FObject obj, int_t df)
{
    FObject hmap = MakeEqHashMap();

    if (SharedObjectP(obj))
    {
        if (FindSharedObjects(hmap, obj, 0, 0) == 0)
            WriteGeneric(port, obj, df, (FWriteFn) WriteGeneric, 0);
        else
        {
            FWriteSharedCtx ctx;
            ctx.HashMap = hmap;
            ctx.Label = 1;
            WriteSharedObject(port, obj, df, (FWriteFn) WriteSharedObject, &ctx);
        }
    }
    else
        WriteGeneric(port, obj, df, (FWriteFn) WriteGeneric, 0);
}

static void WritePair(FObject port, FObject obj, int_t df, FWriteFn wfn, void * ctx)
{
    FAssert(PairP(obj));

    WriteCh(port, '(');
    for (;;)
    {
        wfn(port, First(obj), df, (void *) wfn, ctx);
        if (PairP(Rest(obj)))
        {
            WriteCh(port, ' ');
            obj = Rest(obj);
        }
        else if (Rest(obj) == EmptyListObject)
        {
            WriteCh(port, ')');
            break;
        }
        else
        {
            WriteStringC(port, " . ");
            wfn(port, Rest(obj), df, (void *) wfn, ctx);
            WriteCh(port, ')');
            break;
        }
    }
}

static void WriteLocation(FObject port, FObject obj)
{
    if (PairP(obj))
    {
        if (IdentifierP(First(obj)))
            obj = First(obj);
        else if (PairP(First(obj)) && IdentifierP(First(First(obj))))
            obj = First(First(obj));
    }

    if (IdentifierP(obj) && FixnumP(AsIdentifier(obj)->LineNumber)
            && AsFixnum(AsIdentifier(obj)->LineNumber) > 0)
    {
        FCh s[16];
        int_t sl = FixnumAsString(AsFixnum(AsIdentifier(obj)->LineNumber), s, 10);

        WriteStringC(port, " line: ");
        WriteString(port, s, sl);
    }
}

static void WriteRecord(FObject port, FObject obj, int_t df, FWriteFn wfn, void * ctx)
{
    if (BindingP(obj))
        WriteGeneric(port, AsBinding(obj)->Identifier, df, wfn, ctx);
    else if (ReferenceP(obj))
        WriteGeneric(port, AsReference(obj)->Identifier, df, wfn, ctx);
    else
    {
        FObject rt = AsGenericRecord(obj)->Fields[0];
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<(");
        wfn(port, RecordTypeName(rt), df, (void *) wfn, ctx);
        WriteStringC(port, ": #x");
        WriteString(port, s, sl);

        if (LibraryP(obj))
        {
            WriteCh(port, ' ');
            WriteGeneric(port, AsLibrary(obj)->Name, df, wfn, ctx);
        }
        else if (GlobalP(obj))
        {
            WriteCh(port, ' ');
            WriteGeneric(port, AsGlobal(obj)->Name, df, wfn, ctx);
            WriteCh(port, ' ');
            WriteGeneric(port, AsGlobal(obj)->Module, df, wfn, ctx);
        }
        else if (ExceptionP(obj))
        {
            WriteCh(port, ' ');
            WriteGeneric(port, AsException(obj)->Type, df, wfn, ctx);

            if (SymbolP(AsException(obj)->Who))
            {
                WriteCh(port, ' ');
                WriteGeneric(port, AsException(obj)->Who, df, wfn, ctx);
            }

            WriteCh(port, ' ');
            WriteGeneric(port, AsException(obj)->Message, df, wfn, ctx);

            WriteStringC(port, " irritants: ");
            WriteGeneric(port, AsException(obj)->Irritants, df, wfn, ctx);

            WriteLocation(port, AsException(obj)->Irritants);
        }
        else if (EnvironmentP(obj))
        {
            WriteCh(port, ' ');
            wfn(port, AsEnvironment(obj)->Name, df, (void *) wfn, ctx);
        }
        else if (LambdaP(obj))
        {
            WriteCh(port, ' ');
            WriteGeneric(port, AsLambda(obj)->Name, df, wfn, ctx);
            WriteCh(port, ' ');
            WriteGeneric(port, AsLambda(obj)->Bindings, df, wfn, ctx);
            if (StringP(AsLambda(obj)->Filename) && FixnumP(AsLambda(obj)->LineNumber))
            {
                WriteCh(port, ' ');
                WriteGeneric(port, AsLambda(obj)->Filename, 1, wfn, ctx);
                WriteCh(port, '[');
                WriteGeneric(port, AsLambda(obj)->LineNumber, df, wfn, ctx);
                WriteCh(port, ']');
            }
        }
        else
        {
            for (uint_t fdx = 1; fdx < RecordNumFields(obj); fdx++)
            {
                WriteCh(port, ' ');
                wfn(port, AsRecordType(rt)->Fields[fdx], df, (void *) wfn, ctx);
                WriteStringC(port, ": ");
                wfn(port, AsGenericRecord(obj)->Fields[fdx], df, (void *) wfn, ctx);
            }

        }

        WriteStringC(port, ")>");
    }
}

static void WriteObject(FObject port, FObject obj, int_t df, FWriteFn wfn, void * ctx)
{
    switch (IndirectTag(obj))
    {
    case BoxTag:
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<(box: #x");
        WriteString(port, s, sl);
        WriteCh(port, ' ');
        wfn(port, Unbox(obj), df, (void *) wfn, ctx);
        WriteStringC(port, ")>");
        break;
    }

    case StringTag:
        if (df)
            WriteString(port, AsString(obj)->String, StringLength(obj));
        else
        {
            WriteCh(port, '"');

            for (uint_t idx = 0; idx < StringLength(obj); idx++)
            {
                FCh ch = AsString(obj)->String[idx];
                if (ch == '\\' || ch == '"')
                    WriteCh(port, '\\');
                WriteCh(port, ch);
            }

            WriteCh(port, '"');
        }
        break;

    case CStringTag:
    {
        const char * s = AsCString(obj)->String;

        if (df)
        {
            while (*s)
            {
                WriteCh(port, *s);
                s += 1;
            }
        }
        else
        {
            WriteCh(port, '"');

            while (*s)
            {
                if (*s == '\\' || *s == '"')
                    WriteCh(port, '\\');
                WriteCh(port, *s);
                s += 1;
            }

            WriteCh(port, '"');
        }
        break;
    }

    case VectorTag:
    {
        WriteStringC(port, "#(");
        for (uint_t idx = 0; idx < VectorLength(obj); idx++)
        {
            if (idx > 0)
                WriteCh(port, ' ');
            wfn(port, AsVector(obj)->Vector[idx], df, (void *) wfn, ctx);
        }

        WriteCh(port, ')');
        break;
    }

    case BytevectorTag:
    {
        FCh s[8];
        int_t sl;

        WriteStringC(port, "#u8(");
        for (uint_t idx = 0; idx < BytevectorLength(obj); idx++)
        {
            if (idx > 0)
                WriteCh(port, ' ');

            sl = FixnumAsString((FFixnum) AsBytevector(obj)->Vector[idx], s, 10);
            WriteString(port, s, sl);
        }

        WriteCh(port, ')');
        break;
    }

    case BinaryPortTag:
    case TextualPortTag:
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<");
        if (TextualPortP(obj))
            WriteStringC(port, "textual-");
        else
        {
            FAssert(BinaryPortP(obj));

            WriteStringC(port, "binary-");
        }

        if (InputPortP(obj))
            WriteStringC(port, "input-");
        if (OutputPortP(obj))
            WriteStringC(port, "output-");
        WriteStringC(port, "port: #x");
        WriteString(port, s, sl);

        if (InputPortOpenP(obj) == 0 && OutputPortOpenP(obj) == 0)
            WriteStringC(port, " closed");

        if (StringP(AsGenericPort(obj)->Name))
        {
            WriteCh(port, ' ');
            WriteString(port, AsString(AsGenericPort(obj)->Name)->String,
                    StringLength(AsGenericPort(obj)->Name));
        }

        if (InputPortP(obj))
        {
            if (BinaryPortP(obj))
            {
                WriteStringC(port, " offset: ");
                sl = FixnumAsString(GetOffset(obj), s, 10);
                WriteString(port, s, sl);
            }
            else
            {
                FAssert(TextualPortP(obj));

                WriteStringC(port, " line: ");
                sl = FixnumAsString(GetLineColumn(obj, 0), s, 10);
                WriteString(port, s, sl);
            }
        }

        WriteCh(port, '>');
        break;
    }

    case ProcedureTag:
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<procedure: ");
        WriteString(port, s, sl);

        if (AsProcedure(obj)->Name != NoValueObject)
        {
            WriteCh(port, ' ');
            wfn(port, AsProcedure(obj)->Name, df, (void *) wfn, ctx);
        }

        if (AsProcedure(obj)->Flags & PROCEDURE_FLAG_CLOSURE)
            WriteStringC(port, " closure");

        if (AsProcedure(obj)->Flags & PROCEDURE_FLAG_PARAMETER)
            WriteStringC(port, " parameter");

        if (AsProcedure(obj)->Flags & PROCEDURE_FLAG_CONTINUATION)
            WriteStringC(port, " continuation");

        if (StringP(AsProcedure(obj)->Filename) && FixnumP(AsProcedure(obj)->LineNumber))
        {
            WriteCh(port, ' ');
            WriteGeneric(port, AsProcedure(obj)->Filename, 1, wfn, ctx);
            WriteCh(port, '[');
            WriteGeneric(port, AsProcedure(obj)->LineNumber, df, wfn, ctx);
            WriteCh(port, ']');
        }

//        WriteCh(port, ' ');
//        wfn(port, AsProcedure(obj)->Code, df, wfn, ctx);
        WriteCh(port, '>');
        break;
    }

    case IdentifierTag:
        obj = AsIdentifier(obj)->Symbol;

        FAssert(SymbolP(obj));

        /* Fall through */

    case SymbolTag:
        if (StringP(AsSymbol(obj)->String))
            WriteString(port, AsString(AsSymbol(obj)->String)->String,
                    StringLength(AsSymbol(obj)->String));
        else
        {
            FAssert(CStringP(AsSymbol(obj)->String));

            WriteStringC(port, AsCString(AsSymbol(obj)->String)->String);
        }
        break;

    case RecordTypeTag:
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<record-type: #x");
        WriteString(port, s, sl);
        WriteCh(port, ' ');
        wfn(port, RecordTypeName(obj), df, (void *) wfn, ctx);

        for (uint_t fdx = 1; fdx < RecordTypeNumFields(obj); fdx += 1)
        {
            WriteCh(port, ' ');
            wfn(port, AsRecordType(obj)->Fields[fdx], df, (void *) wfn, ctx);
        }

        WriteStringC(port, ">");
        break;
    }

    case RecordTag:
        WriteRecord(port, obj, df, wfn, ctx);
        break;

    case PrimitiveTag:
    {
        WriteStringC(port, "#<primitive: ");
        WriteStringC(port, AsPrimitive(obj)->Name);
        WriteCh(port, ' ');

        const char * fn = AsPrimitive(obj)->Filename;
        const char * p = fn;
        while (*p != 0)
        {
            if (*p == '/' || *p == '\\')
                fn = p + 1;

            p += 1;
        }

        WriteStringC(port, fn);
        WriteCh(port, '@');
        FCh s[16];
        int_t sl = FixnumAsString(AsPrimitive(obj)->LineNumber, s, 10);
        WriteString(port, s, sl);
        WriteCh(port, '>');
        break;
    }

    case ThreadTag:
        WriteThread(port, obj, df);
        break;

    case ExclusiveTag:
        WriteExclusive(port, obj, df);
        break;

    case ConditionTag:
        WriteCondition(port, obj, df);
        break;

    case HashTreeTag:
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<hash-tree: ");
        WriteString(port, s, sl);
        WriteCh(port, '>');
        break;
    }

    default:
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<unknown: ");
        WriteString(port, s, sl);
        WriteCh(port, '>');
        break;
    }

    }
}

void WriteGeneric(FObject port, FObject obj, int_t df, FWriteFn wfn, void * ctx)
{
    if (NumberP(obj))
        Write(port, NumberToString(obj, 10), 1);
    else if (CharacterP(obj))
    {
        if (AsCharacter(obj) < 128)
        {
            if (df == 0)
                WriteStringC(port, "#\\");
            WriteCh(port, AsCharacter(obj));
        }
        else
        {
            if (df)
                WriteCh(port, AsCharacter(obj));
            else
            {
                FCh s[16];
                int_t sl = FixnumAsString(AsCharacter(obj), s, 16);
                WriteStringC(port, "#\\x");
                WriteString(port, s, sl);
            }
        }
    }
    else if (SpecialSyntaxP(obj))
        WriteSpecialSyntax(port, obj, df);
    else if (InstructionP(obj))
        WriteInstruction(port, obj, df);
    else if (ValuesCountP(obj))
    {
        WriteStringC(port, "#<values-count: ");

        FCh s[16];
        int_t sl = FixnumAsString(AsValuesCount(obj), s, 10);
        WriteString(port, s, sl);

        WriteCh(port, '>');
    }
    else if (PairP(obj))
        WritePair(port, obj, df, wfn, ctx);
    else if (ObjectP(obj))
        WriteObject(port, obj, df, wfn, ctx);
    else if (obj == EmptyListObject)
        WriteStringC(port, "()");
    else if (obj == FalseObject)
        WriteStringC(port, "#f");
    else if (obj == TrueObject)
        WriteStringC(port, "#t");
    else if (obj == EndOfFileObject)
        WriteStringC(port, "#<end-of-file>");
    else if (obj == NoValueObject)
        WriteStringC(port, "#<no-value>");
    else if (obj == WantValuesObject)
        WriteStringC(port, "#<want-values>");
    else if (obj == NotFoundObject)
        WriteStringC(port, "#<not-found>");
    else if (obj == MatchAnyObject)
        WriteStringC(port, "#<match-any>");
    else
    {
        FCh s[16];
        int_t sl = FixnumAsString((FFixnum) obj, s, 16);

        WriteStringC(port, "#<unknown: ");
        WriteString(port, s, sl);
        WriteCh(port, '>');
    }
}

void WriteSimple(FObject port, FObject obj, int_t df)
{
//    FAssert(OutputPortP(port) && AsPort(port)->Context != 0);

    WriteGeneric(port, obj, df, (FWriteFn) WriteGeneric, 0);
}

// ---- Primitives ----

Define("write", WritePrimitive)(int_t argc, FObject argv[])
{
    OneOrTwoArgsCheck("write", argc);
    FObject port = (argc == 2 ? argv[1] : CurrentOutputPort());
    TextualOutputPortArgCheck("write", port);

    Write(port, argv[0], 0);
    return(NoValueObject);
}

Define("write-shared", WriteSharedPrimitive)(int_t argc, FObject argv[])
{
    OneOrTwoArgsCheck("write-shared", argc);
    FObject port = (argc == 2 ? argv[1] : CurrentOutputPort());
    TextualOutputPortArgCheck("write-shared", port);

    WriteShared(port, argv[0], 0);
    return(NoValueObject);
}

Define("write-simple", WriteSimplePrimitive)(int_t argc, FObject argv[])
{
    OneOrTwoArgsCheck("write-simple", argc);
    FObject port = (argc == 2 ? argv[1] : CurrentOutputPort());
    TextualOutputPortArgCheck("write-simple", port);

    WriteSimple(port, argv[0], 0);
    return(NoValueObject);
}

Define("display", DisplayPrimitive)(int_t argc, FObject argv[])
{
    OneOrTwoArgsCheck("display", argc);
    FObject port = (argc == 2 ? argv[1] : CurrentOutputPort());
    TextualOutputPortArgCheck("display", port);

    Write(port, argv[0], 1);
    return(NoValueObject);
}

Define("newline", NewlinePrimitive)(int_t argc, FObject argv[])
{
    ZeroOrOneArgsCheck("newline", argc);
    FObject port = (argc == 1 ? argv[0] : CurrentOutputPort());
    TextualOutputPortArgCheck("newline", port);

    WriteCh(port, '\n');
    return(NoValueObject);
}

Define("write-char", WriteCharPrimitive)(int_t argc, FObject argv[])
{
    OneOrTwoArgsCheck("write-char", argc);
    CharacterArgCheck("write-char", argv[0]);
    FObject port = (argc == 2 ? argv[1] : CurrentOutputPort());
    TextualOutputPortArgCheck("write-char", port);

    WriteCh(port, AsCharacter(argv[0]));
    return(NoValueObject);
}

Define("write-string", WriteStringPrimitive)(int_t argc, FObject argv[])
{
    OneToFourArgsCheck("write-string", argc);
    StringArgCheck("write-string", argv[0]);
    FObject port = (argc > 1 ? argv[1] : CurrentOutputPort());
    TextualOutputPortArgCheck("write-string", port);

    int_t strt;
    int_t end;
    if (argc > 2)
    {
        IndexArgCheck("write-string", argv[2], StringLength(argv[0]));

        strt = AsFixnum(argv[2]);

        if (argc > 3)
        {
            EndIndexArgCheck("write-string", argv[3], strt, StringLength(argv[0]));

            end = AsFixnum(argv[3]);
        }
        else
            end = (int_t) StringLength(argv[0]);
    }
    else
    {
        strt = 0;
        end = (int_t) StringLength(argv[0]);
    }

    WriteString(port, AsString(argv[0])->String + strt, end - strt);
    return(NoValueObject);
}

Define("write-u8", WriteU8Primitive)(int_t argc, FObject argv[])
{
    OneOrTwoArgsCheck("write-u8", argc);
    ByteArgCheck("write-u8", argv[0]);
    FObject port = (argc == 2 ? argv[1] : CurrentOutputPort());
    BinaryOutputPortArgCheck("write-u8", port);

    FByte b = (FByte) AsFixnum(argv[0]);
    WriteBytes(port, &b, 1);
    return(NoValueObject);
}

Define("write-bytevector", WriteBytevectorPrimitive)(int_t argc, FObject argv[])
{
    OneToFourArgsCheck("write-bytevector", argc);
    BytevectorArgCheck("write-bytevector", argv[0]);
    FObject port = (argc > 1 ? argv[1] : CurrentOutputPort());
    BinaryOutputPortArgCheck("write-bytevector", port);

    int_t strt;
    int_t end;
    if (argc > 2)
    {
        IndexArgCheck("write-bytevector", argv[2], BytevectorLength(argv[0]));

        strt = AsFixnum(argv[2]);

        if (argc > 3)
        {
            EndIndexArgCheck("write-bytevector", argv[3], strt, BytevectorLength(argv[0]));

            end = AsFixnum(argv[3]);
        }
        else
            end = (int_t) BytevectorLength(argv[0]);
    }
    else
    {
        strt = 0;
        end = (int_t) BytevectorLength(argv[0]);
    }

    WriteBytes(port, AsBytevector(argv[0])->Vector + strt, end - strt);
    return(NoValueObject);
}

Define("flush-output-port", FlushOutputPortPrimitive)(int_t argc, FObject argv[])
{
    ZeroOrOneArgsCheck("flush-output-port", argc);
    FObject port = (argc == 1 ? argv[0] : CurrentOutputPort());
    OutputPortArgCheck("flush-output-port", port);

    FlushOutput(port);
    return(NoValueObject);
}

static FObject Primitives[] =
{
    WritePrimitive,
    WriteSharedPrimitive,
    WriteSimplePrimitive,
    DisplayPrimitive,
    NewlinePrimitive,
    WriteCharPrimitive,
    WriteStringPrimitive,
    WriteU8Primitive,
    WriteBytevectorPrimitive,
    FlushOutputPortPrimitive
};

void SetupWrite()
{
    for (uint_t idx = 0; idx < sizeof(Primitives) / sizeof(FPrimitive *); idx++)
        DefinePrimitive(R.Bedrock, R.BedrockLibrary, Primitives[idx]);
}
