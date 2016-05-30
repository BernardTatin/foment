/*

Foment

*/

#include "foment.hpp"
#include "unicode.hpp"

// ---- Characters ----

Define("char?", CharPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char?", argc);

    return(CharacterP(argv[0]) ? TrueObject : FalseObject);
}

Define("char=?", CharEqualPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char=?", argc);
    CharacterArgCheck("char=?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char=?", argv[adx]);

        if (AsCharacter(argv[adx - 1]) != AsCharacter(argv[adx]))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char<?", CharLessThanPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char<?", argc);
    CharacterArgCheck("char<?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char<?", argv[adx]);

        if (AsCharacter(argv[adx - 1]) >= AsCharacter(argv[adx]))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char>?", CharGreaterThanPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char>?", argc);
    CharacterArgCheck("char>?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char>?", argv[adx]);

        if (AsCharacter(argv[adx - 1]) <= AsCharacter(argv[adx]))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char<=?", CharLessThanEqualPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char<=?", argc);
    CharacterArgCheck("char<=?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char<=?", argv[adx]);

        if (AsCharacter(argv[adx - 1]) > AsCharacter(argv[adx]))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char>=?", CharGreaterThanEqualPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char>=?", argc);
    CharacterArgCheck("char>=?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char>=?", argv[adx]);

        if (AsCharacter(argv[adx - 1]) < AsCharacter(argv[adx]))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char-ci=?", CharCiEqualPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char-ci=?", argc);
    CharacterArgCheck("char-ci=?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char-ci=?", argv[adx]);

        if (CharFoldcase(AsCharacter(argv[adx - 1])) != CharFoldcase(AsCharacter(argv[adx])))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char-ci<?", CharCiLessThanPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char-ci<?", argc);
    CharacterArgCheck("char-ci<?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char-ci<?", argv[adx]);

        if (CharFoldcase(AsCharacter(argv[adx - 1])) >= CharFoldcase(AsCharacter(argv[adx])))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char-ci>?", CharCiGreaterThanPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char-ci>?", argc);
    CharacterArgCheck("char-ci>?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char-ci>?", argv[adx]);

        if (CharFoldcase(AsCharacter(argv[adx - 1])) <= CharFoldcase(AsCharacter(argv[adx])))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char-ci<=?", CharCiLessThanEqualPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char-ci<=?", argc);
    CharacterArgCheck("char-ci<=?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char-ci<=?", argv[adx]);

        if (CharFoldcase(AsCharacter(argv[adx - 1])) > CharFoldcase(AsCharacter(argv[adx])))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char-ci>=?", CharCiGreaterThanEqualPPrimitive)(int_t argc, FObject argv[])
{
    AtLeastTwoArgsCheck("char-ci>=?", argc);
    CharacterArgCheck("char-ci>=?", argv[0]);

    for (int_t adx = 1; adx < argc; adx++)
    {
        CharacterArgCheck("char-ci>=?", argv[adx]);

        if (CharFoldcase(AsCharacter(argv[adx - 1])) < CharFoldcase(AsCharacter(argv[adx])))
            return(FalseObject);
    }

    return(TrueObject);
}

Define("char-alphabetic?", CharAlphabeticPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-alphabetic?", argc);
    CharacterArgCheck("char-alphabetic?", argv[0]);

    return(AlphabeticP(AsCharacter(argv[0])) ? TrueObject : FalseObject);
}

Define("char-numeric?", CharNumericPPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-numeric?", argc);
    CharacterArgCheck("char-numeric?", argv[0]);

    int_t dv = DigitValue(AsCharacter(argv[0]));
    if (dv < 0 || dv > 9)
        return(FalseObject);
    return(TrueObject);
}

Define("char-whitespace?", CharWhitespacePPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-whitespace?", argc);
    CharacterArgCheck("char-whitespace?", argv[0]);

    return(WhitespaceP(AsCharacter(argv[0])) ? TrueObject : FalseObject);
}

Define("char-upper-case?", CharUpperCasePPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-upper-case?", argc);
    CharacterArgCheck("char-upper-case?", argv[0]);

    return(UppercaseP(AsCharacter(argv[0])) ? TrueObject : FalseObject);
}

Define("char-lower-case?", CharLowerCasePPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-lower-case?", argc);
    CharacterArgCheck("char-lower-case?", argv[0]);

    return(LowercaseP(AsCharacter(argv[0])) ? TrueObject : FalseObject);
}

Define("digit-value", DigitValuePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("digit-value", argc);
    CharacterArgCheck("digit-value", argv[0]);

    int_t dv = DigitValue(AsCharacter(argv[0]));
    if (dv < 0 || dv > 9)
        return(FalseObject);
    return(MakeFixnum(dv));
}

Define("char->integer", CharToIntegerPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char->integer", argc);
    CharacterArgCheck("char->integer", argv[0]);

    return(MakeFixnum(AsCharacter(argv[0])));
}

Define("integer->char", IntegerToCharPrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("integer->char", argc);
    FixnumArgCheck("integer->char", argv[0]);

    return(MakeCharacter(AsFixnum(argv[0])));
}

Define("char-upcase", CharUpcasePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-upcase", argc);
    CharacterArgCheck("char-upcase", argv[0]);

    return(MakeCharacter(CharUpcase(AsCharacter(argv[0]))));
}

Define("char-downcase", CharDowncasePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-downcase", argc);
    CharacterArgCheck("char-downcase", argv[0]);

    return(MakeCharacter(CharDowncase(AsCharacter(argv[0]))));
}

Define("char-foldcase", CharFoldcasePrimitive)(int_t argc, FObject argv[])
{
    OneArgCheck("char-foldcase", argc);
    CharacterArgCheck("char-foldcase", argv[0]);

    return(MakeCharacter(CharFoldcase(AsCharacter(argv[0]))));
}

Define("char-compare", CharComparePrimitive)(int_t argc, FObject argv[])
{
    TwoArgsCheck("char-compare", argc);
    CharacterArgCheck("char-compare", argv[0]);
    CharacterArgCheck("char-compare", argv[1]);

    if (argv[0] == argv[1])
        return(MakeFixnum(0));
    return(AsCharacter(argv[0]) < AsCharacter(argv[1]) ? MakeFixnum(-1) : MakeFixnum(1));
}

Define("char-ci-compare", CharCiComparePrimitive)(int_t argc, FObject argv[])
{
    TwoArgsCheck("char-ci-compare", argc);
    CharacterArgCheck("char-ci-compare", argv[0]);
    CharacterArgCheck("char-ci-compare", argv[1]);

    FCh ch0 = CharFoldcase(AsCharacter(argv[0]));
    FCh ch1 = CharFoldcase(AsCharacter(argv[1]));

    if (ch0 == ch1)
        return(MakeFixnum(0));
    return(ch0 < ch1 ? MakeFixnum(-1) : MakeFixnum(1));
}

static FObject Primitives[] =
{
    CharPPrimitive,
    CharEqualPPrimitive,
    CharLessThanPPrimitive,
    CharGreaterThanPPrimitive,
    CharLessThanEqualPPrimitive,
    CharGreaterThanEqualPPrimitive,
    CharCiEqualPPrimitive,
    CharCiLessThanPPrimitive,
    CharCiGreaterThanPPrimitive,
    CharCiLessThanEqualPPrimitive,
    CharCiGreaterThanEqualPPrimitive,
    CharAlphabeticPPrimitive,
    CharNumericPPrimitive,
    CharWhitespacePPrimitive,
    CharUpperCasePPrimitive,
    CharLowerCasePPrimitive,
    DigitValuePrimitive,
    CharToIntegerPrimitive,
    IntegerToCharPrimitive,
    CharUpcasePrimitive,
    CharDowncasePrimitive,
    CharFoldcasePrimitive,
    CharCiComparePrimitive
};

void SetupCharacters()
{
    DefineComparator("char-comparator", CharPPrimitive, CharEqualPPrimitive,
            CharComparePrimitive, EqHashPrimitive);

    for (uint_t idx = 0; idx < sizeof(Primitives) / sizeof(FPrimitive *); idx++)
        DefinePrimitive(R.Bedrock, R.BedrockLibrary, Primitives[idx]);
}
