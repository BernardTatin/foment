/*

Foment

*/

#include "foment.hpp"
#include "compile.hpp"

EternalSymbol(TagSymbol, "tag");
EternalSymbol(UsePassSymbol, "use-pass");
EternalSymbol(ConstantPassSymbol, "constant-pass");
EternalSymbol(AnalysisPassSymbol, "analysis-pass");

// ---- SyntacticEnv ----

static const char * SyntacticEnvFieldsC[] = {"global-bindings", "local-bindings"};

FObject MakeSyntacticEnv(FObject obj)
{
    FAssert(sizeof(FSyntacticEnv) == sizeof(SyntacticEnvFieldsC) + sizeof(FRecord));
    FAssert(EnvironmentP(obj) || SyntacticEnvP(obj));

    FSyntacticEnv * se = (FSyntacticEnv *) MakeRecord(R.SyntacticEnvRecordType);
    if (EnvironmentP(obj))
    {
        se->GlobalBindings = obj;
        se->LocalBindings = EmptyListObject;
    }
    else
    {
        se->GlobalBindings = AsSyntacticEnv(obj)->GlobalBindings;
        se->LocalBindings = AsSyntacticEnv(obj)->LocalBindings;
    }

    return(se);
}

// ---- Binding ----

static const char * BindingFieldsC[] = {"identifier", "syntax", "syntactic-env", "rest-arg",
    "use-count", "set-count", "escapes", "level", "slot", "constant"};

FObject MakeBinding(FObject se, FObject id, FObject ra)
{
    FAssert(sizeof(FBinding) == sizeof(BindingFieldsC) + sizeof(FRecord));
    FAssert(SyntacticEnvP(se));
    FAssert(IdentifierP(id));
    FAssert(ra == TrueObject || ra == FalseObject);

    FBinding * b = (FBinding *) MakeRecord(R.BindingRecordType);
    b->Identifier = id;
    b->Syntax = NoValueObject;
    b->SyntacticEnv = se;
    b->RestArg = ra;

    b->UseCount = MakeFixnum(0);
    b->SetCount = MakeFixnum(0);
    b->Escapes = FalseObject;
    b->Level = MakeFixnum(0);
    b->Slot = MakeFixnum(-1);
    b->Constant = NoValueObject;

    return(b);
}

// ---- Identifier ----

static int_t IdentifierMagic = 0;

FObject MakeIdentifier(FObject sym, FObject fn, int_t ln)
{
    FAssert(SymbolP(sym));

    FIdentifier * nid = (FIdentifier *) MakeObject(IdentifierTag, sizeof(FIdentifier), 4, "foobar");
    nid->Symbol = sym;
    nid->Filename = fn;
    nid->SyntacticEnv = NoValueObject;
    nid->Wrapped = NoValueObject;

    nid->LineNumber = ln;
    IdentifierMagic += 1;
    nid->Magic = IdentifierMagic;

    return(nid);
}

FObject MakeIdentifier(FObject sym)
{
    return(MakeIdentifier(sym, NoValueObject, 0));
}

FObject WrapIdentifier(FObject id, FObject se)
{
    FAssert(IdentifierP(id));
    FAssert(SyntacticEnvP(se));

    FIdentifier * nid = (FIdentifier *) MakeObject(IdentifierTag, sizeof(FIdentifier), 4, "foobar");
    nid->Symbol = AsIdentifier(id)->Symbol;
    nid->Filename = AsIdentifier(id)->Filename;
    nid->SyntacticEnv = se;
    nid->Wrapped = id;
    nid->LineNumber = AsIdentifier(id)->LineNumber;
    nid->Magic = AsIdentifier(id)->Magic;

    return(nid);
}

// ---- Lambda ----

static const char * LambdaFieldsC[] = {"name", "bindings", "body", "rest-arg", "arg-count",
    "escapes", "use-stack", "level", "slot-count", "middle-pass", "may-inline", "procedure",
    "body-index", "filename", "line-number"};

FObject MakeLambda(FObject enc, FObject nam, FObject bs, FObject body)
{
    FAssert(sizeof(FLambda) == sizeof(LambdaFieldsC) + sizeof(FRecord));
    FAssert(LambdaP(enc) || enc == NoValueObject);

    FLambda * l = (FLambda *) MakeRecord(R.LambdaRecordType);
    l->Name = nam;
    l->Bindings = bs;
    l->Body = body;

    l->RestArg = FalseObject;
    l->ArgCount = MakeFixnum(0);

    l->Escapes = FalseObject;
    l->UseStack = TrueObject;
    l->Level = LambdaP(enc) ? MakeFixnum(AsFixnum(AsLambda(enc)->Level) + 1) : MakeFixnum(1);
    l->SlotCount = MakeFixnum(-1);
    l->CompilerPass = NoValueObject;
    l->MayInline = MakeFixnum(0); // Number of procedure calls or FalseObject.

    l->Procedure = NoValueObject;
    l->BodyIndex = NoValueObject;

    if (IdentifierP(nam))
    {
        l->Filename = AsIdentifier(nam)->Filename;
        l->LineNumber = MakeFixnum(AsIdentifier(nam)->LineNumber);
    }
    else
    {
        l->Filename = NoValueObject;
        l->LineNumber = NoValueObject;
    }

    return(l);
}

// ---- CaseLambda ----

static const char * CaseLambdaFieldsC[] = {"cases", "name", "escapes"};

FObject MakeCaseLambda(FObject cases)
{
    FAssert(sizeof(FCaseLambda) == sizeof(CaseLambdaFieldsC) + sizeof(FRecord));

    FCaseLambda * cl = (FCaseLambda *) MakeRecord(R.CaseLambdaRecordType);
    cl->Cases = cases;
    cl->Name = NoValueObject;
    cl->Escapes = FalseObject;

    return(cl);
}

// ---- InlineVariable ----

static const char * InlineVariableFieldsC[] = {"index"};

FObject MakeInlineVariable(int_t idx)
{
    FAssert(sizeof(FInlineVariable) == sizeof(InlineVariableFieldsC) + sizeof(FRecord));

    FAssert(idx >= 0);

    FInlineVariable * iv = (FInlineVariable *) MakeRecord(R.InlineVariableRecordType);
    iv->Index = MakeFixnum(idx);

    return(iv);
}

// ---- Reference ----

static const char * ReferenceFieldsC[] = {"binding", "identifier"};

FObject MakeReference(FObject be, FObject id)
{
    FAssert(sizeof(FReference) == sizeof(ReferenceFieldsC) + sizeof(FRecord));
    FAssert(BindingP(be) || EnvironmentP(be));
    FAssert(IdentifierP(id));

    FReference * r = (FReference *) MakeRecord(R.ReferenceRecordType);
    r->Binding = be;
    r->Identifier = id;

    return(r);
}

// ----------------

FObject CompileLambda(FObject env, FObject name, FObject formals, FObject body)
{
    FObject obj = SPassLambda(NoValueObject, MakeSyntacticEnv(env), name, formals, body);
    FAssert(LambdaP(obj));

    UPassLambda(AsLambda(obj), 1);
    CPassLambda(AsLambda(obj));
    APassLambda(0, AsLambda(obj));
    return(GPassLambda(AsLambda(obj)));
}

FObject GetInteractionEnv()
{
    if (EnvironmentP(R.InteractionEnv) == 0)
    {
        R.InteractionEnv = MakeEnvironment(StringCToSymbol("interaction"), TrueObject);
        EnvironmentImportLibrary(R.InteractionEnv,
                List(StringCToSymbol("foment"), StringCToSymbol("base")));
    }

    return(R.InteractionEnv);
}

// ----------------

Define("%compile-eval", CompileEvalPrimitive)(int_t argc, FObject argv[])
{
    FMustBe(argc == 2);
    EnvironmentArgCheck("eval", argv[1]);

    return(CompileEval(argv[0], argv[1]));
}

Define("interaction-environment", InteractionEnvironmentPrimitive)(int_t argc, FObject argv[])
{
    if (argc == 0)
    {
        ZeroArgsCheck("interaction-environment", argc);

        return(GetInteractionEnv());
    }

    FObject env = MakeEnvironment(StringCToSymbol("interaction"), TrueObject);

    for (int_t adx = 0; adx < argc; adx++)
    {
        ListArgCheck("environment", argv[adx]);

        EnvironmentImportSet(env, argv[adx], argv[adx]);
    }

    return(env);
}

Define("environment", EnvironmentPrimitive)(int_t argc, FObject argv[])
{
    FObject env = MakeEnvironment(EmptyListObject, FalseObject);

    for (int_t adx = 0; adx < argc; adx++)
    {
        ListArgCheck("environment", argv[adx]);

        EnvironmentImportSet(env, argv[adx], argv[adx]);
    }

    EnvironmentImmutable(env);
    return(env);
}

Define("syntax", SyntaxPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("syntax", argc);

    return(ExpandExpression(NoValueObject, MakeSyntacticEnv(GetInteractionEnv()), argv[0]));
}

Define("unsyntax", UnsyntaxPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("unsyntax", argc);

    return(SyntaxToDatum(argv[0]));
}

static FPrimitive * Primitives[] =
{
    &CompileEvalPrimitive,
    &InteractionEnvironmentPrimitive,
    &EnvironmentPrimitive,
    &SyntaxPrimitive,
    &UnsyntaxPrimitive
};

void SetupCompile()
{
    R.SyntacticEnvRecordType = MakeRecordTypeC("syntactic-env",
            sizeof(SyntacticEnvFieldsC) / sizeof(char *), SyntacticEnvFieldsC);
    R.BindingRecordType = MakeRecordTypeC("binding", sizeof(BindingFieldsC) / sizeof(char *),
            BindingFieldsC);
    R.ReferenceRecordType = MakeRecordTypeC("reference",
            sizeof(ReferenceFieldsC) / sizeof(char *), ReferenceFieldsC);
    R.LambdaRecordType = MakeRecordTypeC("lambda", sizeof(LambdaFieldsC) / sizeof(char *),
            LambdaFieldsC);
    R.CaseLambdaRecordType = MakeRecordTypeC("case-lambda",
            sizeof(CaseLambdaFieldsC) / sizeof(char *), CaseLambdaFieldsC);
    R.InlineVariableRecordType = MakeRecordTypeC("inline-variable",
            sizeof(InlineVariableFieldsC) / sizeof(char *), InlineVariableFieldsC);

    R.ElseReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("else")));
    R.ArrowReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("=>")));
    R.LibraryReference = MakeReference(R.Bedrock,
            MakeIdentifier(StringCToSymbol("library")));
    R.AndReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("and")));
    R.OrReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("or")));
    R.NotReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("not")));
    R.QuasiquoteReference = MakeReference(R.Bedrock,
            MakeIdentifier(StringCToSymbol("quasiquote")));
    R.UnquoteReference = MakeReference(R.Bedrock,
            MakeIdentifier(StringCToSymbol("unquote")));
    R.UnquoteSplicingReference = MakeReference(R.Bedrock,
            MakeIdentifier(StringCToSymbol("unquote-splicing")));
    R.ConsReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("cons")));
    R.AppendReference = MakeReference(R.Bedrock,
            MakeIdentifier(StringCToSymbol("append")));
    R.ListToVectorReference = MakeReference(R.Bedrock,
            MakeIdentifier(StringCToSymbol("list->vector")));
    R.EllipsisReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("...")));
    R.UnderscoreReference = MakeReference(R.Bedrock, MakeIdentifier(StringCToSymbol("_")));

    InternSymbol(TagSymbol);
    InternSymbol(UsePassSymbol);
    InternSymbol(ConstantPassSymbol);
    InternSymbol(AnalysisPassSymbol);

    FAssert(TagSymbol == StringCToSymbol("tag"));
    FAssert(UsePassSymbol == StringCToSymbol("use-pass"));
    FAssert(ConstantPassSymbol == StringCToSymbol("constant-pass"));
    FAssert(AnalysisPassSymbol == StringCToSymbol("analysis-pass"));
    FAssert(R.InteractionEnv == NoValueObject);

    SetupSyntaxRules();

    for (uint_t idx = 0; idx < sizeof(Primitives) / sizeof(FPrimitive *); idx++)
        DefinePrimitive(R.Bedrock, R.BedrockLibrary, Primitives[idx]);
}
