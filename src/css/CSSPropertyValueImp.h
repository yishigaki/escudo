/*
 * Copyright 2010-2013 Esrille Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CSSSPROPERTYVALUE_IMP_H
#define CSSSPROPERTYVALUE_IMP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/Document.h>
#include <org/w3c/dom/Element.h>

#include <assert.h>  // TODO
#include <math.h>

#include "CSSValueParser.h"
#include "CSSSerialize.h"
#include "http/HTTPRequest.h"

class FontTexture;

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class ContainingBlock;
class BoxImage;
class LineBox;
class InlineBox;
class CSSStyleDeclarationImp;
class ViewCSSImp;

class CSSFontSizeValueImp;

struct CSSNumericValue
{
    static const char16_t* Units[];

    unsigned short unit;  // cf. CSSPrimitiveValue
    short index;  // non-negative value if this value has a keyword index value rather than number
    float number;
    float resolved;

public:
    CSSNumericValue(int index = 0) :
        unit(CSSParserTerm::CSS_TERM_INDEX),
        index(index),
        number(0.0f),
        resolved(NAN)
    {
        assert(0 <= index);
    }
    CSSNumericValue(float number, unsigned short unit = css::CSSPrimitiveValue::CSS_NUMBER) :
        unit(unit),
        index(-1),
        number(number),
        resolved(NAN)
    {}
    std::u16string getCssText(const char16_t* options[] = 0, unsigned short resolvedUnit = css::CSSPrimitiveValue::CSS_PX) const {
        if (isPercentage())
            return getSpecifiedCssText(options);
        else
            return getResolvedCssText(options, resolvedUnit);
    }
    std::u16string getSpecifiedCssText(const char16_t* options[] = 0) const {
        if (unit == CSSParserTerm::CSS_TERM_INDEX)
            return options[index];
        std::u16string cssText = CSSSerializeNumber(number);
        if (css::CSSPrimitiveValue::CSS_PERCENTAGE <= unit && unit <= css::CSSPrimitiveValue::CSS_KHZ)
            cssText += Units[unit - css::CSSPrimitiveValue::CSS_PERCENTAGE];
        return cssText;
    }
    // cf. http://dvcs.w3.org/hg/csswg/raw-file/tip/cssom/Overview.html#resolved-values
    std::u16string getResolvedCssText(const char16_t* options[] = 0, unsigned short resolvedUnit = css::CSSPrimitiveValue::CSS_PX) const {
        if (isnan(resolved))
            return getSpecifiedCssText(options);
        std::u16string cssText = CSSSerializeNumber(resolved);
        if (css::CSSPrimitiveValue::CSS_PERCENTAGE <= resolvedUnit && resolvedUnit <= css::CSSPrimitiveValue::CSS_KHZ)
            cssText += Units[resolvedUnit - css::CSSPrimitiveValue::CSS_PERCENTAGE];
        return cssText;
    }
    CSSNumericValue& setValue(CSSParserTerm* term) {
        assert(term);
        unit = term->unit;
        index = (unit == CSSParserTerm::CSS_TERM_INDEX) ? term->getIndex() : -1;
        number = static_cast<float>(term->number);
        resolved = NAN;
        return *this;
    }
    CSSNumericValue& setValue(float number, unsigned short unit = css::CSSPrimitiveValue::CSS_NUMBER) {
        assert(!isnan(number));
        this->unit = unit;
        this->index = -1;
        this->number = number;
        resolved = NAN;
        return *this;
    }
    bool isNaN() const {
        return isnan(resolved);
    }
    float getValue() const {
        return (!isNaN()) ? resolved : 0.0f;
    }
    float getPx() const {
        return (!isNaN()) ? resolved : 0.0f;
    }
    void setPx(float value) {
        resolved = value;
    }
    bool isNegative() const {
        return !isIndex() && number < 0.0;
    }
    bool isIndex() const {
        return unit == CSSParserTerm::CSS_TERM_INDEX;
    }
    short getIndex() const {
        if (unit == CSSParserTerm::CSS_TERM_INDEX)
            return index;
        return -1;
    }
    bool isLength() const {
        switch (unit) {
        case css::CSSPrimitiveValue::CSS_EMS:
        case css::CSSPrimitiveValue::CSS_EXS:
        case css::CSSPrimitiveValue::CSS_PX:
        case css::CSSPrimitiveValue::CSS_CM:
        case css::CSSPrimitiveValue::CSS_MM:
        case css::CSSPrimitiveValue::CSS_IN:
        case css::CSSPrimitiveValue::CSS_PT:
        case css::CSSPrimitiveValue::CSS_PC:
            return true;
        default:
            return false;
        }
    }
    CSSNumericValue& setIndex(short value) {
        unit = CSSParserTerm::CSS_TERM_INDEX;
        index = value;
        resolved = NAN;
        return *this;
    }
    bool isPercentage() const {
        return unit == css::CSSPrimitiveValue::CSS_PERCENTAGE;
    }
    // Compare as two specified values
    bool operator==(const CSSNumericValue& value) const {
        if (unit != value.unit)
            return false;
        if (unit == CSSParserTerm::CSS_TERM_INDEX)
            return (index == value.index);
        else
            return (number == value.number);
    }
    bool operator!=(const CSSNumericValue& value) const {
        return !(*this == value);
    }
    CSSNumericValue& operator=(const CSSNumericValue& other) {
        unit = other.unit;
        index = other.index;
        number = other.number;
        resolved = other.resolved;
        return *this;
    }

    // Note on specify() vs inherit()
    // Properties like 'font-size' inherits the computed value, which is
    // always an absolute length, and how it is computed is not relevant.
    // On the other hand, properties like 'min-height' inherits the computed
    // value, which can be a percentage or an absolute length. In such cases,
    // the unit, index, and number values still need to be inherited.
    void specify(const CSSNumericValue& value) {
        unit = value.unit;
        index = value.index;
        number = value.number;
        if (!isnan(value.resolved))
            resolved = value.resolved;
        // otherwise, keep the current resolved value.
    }
    void inherit(const CSSNumericValue& value) {
        if (isnan(value.resolved))
            specify(value);
        else {
            unit = css::CSSPrimitiveValue::CSS_PX;
            index = -1;
            resolved = number = value.resolved;
        }
    }
    void inheritLength(const CSSNumericValue& value) {
        if (value.isLength())
            inherit(value);
        else
            specify(value);

    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, CSSStyleDeclarationImp* style, float fullSize);
    void clip(float min, float max) {
        if (!isnan(resolved))
            resolved = std::min(max, std::max(min, resolved));
    }
};

class CSSPropertyValueImp
{
public:
    virtual CSSPropertyValueImp& setValue(CSSParserTerm* term) {
        return *this;
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser) {
        CSSParserTerm* term = parser->getStack().back();
        setValue(term);
        return true;
    }
    // CSSPropertyValue
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return u"";
    }
    virtual void setCssText(const std::u16string& cssText) {}
};

class CSSListStyleTypeValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        None,
        Disc,
        Circle,
        Square,
        Decimal,
        DecimalLeadingZero,
        LowerRoman,
        UpperRoman,
        LowerGreek,
        LowerLatin,
        UpperLatin,
        Armenian,
        Georgian,
        LowerAlpha,
        UpperAlpha
    };
    unsigned getValue() const {
        return value;
    }
    CSSListStyleTypeValueImp& setValue(unsigned value = Disc) {
        this->value = value;
        return *this;
    }
    CSSListStyleTypeValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl = 0) const {
        return Options[value];
    }
    bool operator==(const CSSListStyleTypeValueImp& style) const {
        return value == style.value;
    }
    bool operator!=(const CSSListStyleTypeValueImp& style) const {
        return value != style.value;
    }
    void specify(const CSSListStyleTypeValueImp& specified) {
        value = specified.value;
    }
    CSSListStyleTypeValueImp(unsigned initial = Disc) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSNumericValueImp : public CSSPropertyValueImp
{
protected:
    CSSNumericValue value;
public:
    CSSNumericValueImp& setValue(float number = 0.0f, unsigned short unit = css::CSSPrimitiveValue::CSS_NUMBER) {
        value.setValue(number, unit);
        return *this;
    }
    CSSNumericValueImp& setValue(CSSParserTerm* term) {
        value.setValue(term);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return value.getCssText(0, css::CSSPrimitiveValue::CSS_NUMBER);
    }
    bool operator==(const CSSNumericValueImp& n) {
        return value == n.value;
    }
    bool operator!=(const CSSNumericValueImp& n) {
        return value != n.value;
    }
    void specify(const CSSNumericValueImp& specified) {
        value.specify(specified.value);
    }
    void inherit(const CSSNumericValueImp& parent) {
        value.inheritLength(parent.value);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, CSSStyleDeclarationImp* style, float fullSize);
    void clip(float min, float max);
    float getValue() const {
        return value.getValue();
    }
    float getPx() const {
        return value.getPx();
    }
    bool isPercentage() const {
        return value.isPercentage();
    }
    CSSNumericValueImp(float number = 0.0f, unsigned short unit = css::CSSPrimitiveValue::CSS_NUMBER) :
        value(number, unit) {
    }
};

class CSSNonNegativeLengthImp : public CSSNumericValueImp
{
public:
    CSSNonNegativeLengthImp& setValue(float number = 0.0f, unsigned short unit = css::CSSPrimitiveValue::CSS_PX) {
        value.setValue(number, unit);
        return *this;
    }
    CSSNonNegativeLengthImp& setValue(CSSParserTerm* term) {
        value.setValue(term);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return value.getCssText(0, css::CSSPrimitiveValue::CSS_PX);
    }
    bool operator==(const CSSNonNegativeLengthImp& n) {
        return value == n.value;
    }
    bool operator!=(const CSSNonNegativeLengthImp& n) {
        return value != n.value;
    }
    void specify(const CSSNonNegativeLengthImp& specified) {
        value.specify(specified.value);
    }
    void inherit(const CSSNonNegativeLengthImp& parent) {
        value.inheritLength(parent.value);
    }
    CSSNonNegativeLengthImp(float number = 0.0f, unsigned short unit = css::CSSPrimitiveValue::CSS_PX) :
        CSSNumericValueImp(number, unit) {
    }
};

class CSSPaddingWidthValueImp : public CSSNonNegativeLengthImp
{
public:
    CSSPaddingWidthValueImp& setValue(float number = 0.0f, unsigned short unit = css::CSSPrimitiveValue::CSS_PX) {
        value.setValue(number, unit);
        return *this;
    }
    CSSPaddingWidthValueImp& setValue(CSSParserTerm* term) {
        value.setValue(term);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return value.getResolvedCssText();
    }
    bool operator==(const CSSPaddingWidthValueImp& n) {
        return value == n.value;
    }
    bool operator!=(const CSSPaddingWidthValueImp& n) {
        return value != n.value;
    }
    void specify(const CSSPaddingWidthValueImp& specified) {
        value.specify(specified.value);
    }
    void inherit(const CSSPaddingWidthValueImp& parent) {
        value.inheritLength(parent.value);
    }
    CSSPaddingWidthValueImp(float number = 0.0f, unsigned short unit = css::CSSPrimitiveValue::CSS_PX) :
        CSSNonNegativeLengthImp(number, unit) {
    }
};

// <length>, <percentage>, auto
class CSSAutoLengthValueImp : public CSSPropertyValueImp
{
protected:
    CSSNumericValue length;
public:
    enum {
        Auto,
    };
    CSSAutoLengthValueImp& setValue(float number, unsigned short unit) {
        length.setValue(number, unit);
        return *this;
    }
    CSSAutoLengthValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            length.setValue(term);
        else
            length.setIndex(Auto);
        return *this;
    }
    bool isAuto() const {
        return length.getIndex() == Auto;
    }
    bool isPercentage() const {
        return length.isPercentage();
    }
    float getPercentage() const {
        assert(isPercentage());
        return length.number;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return length.getResolvedCssText(Options);
    }
    bool operator==(const CSSAutoLengthValueImp& value) {
        return length == value.length;
    }
    bool operator!=(const CSSAutoLengthValueImp& value) {
        return length != value.length;
    }
    void specify(const CSSAutoLengthValueImp& specified) {
        length.specify(specified.length);
    }
    void inherit(const CSSAutoLengthValueImp& parent) {
        length.inheritLength(parent.length);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, CSSStyleDeclarationImp* style, float fullSize);
    float getPx() const {
        return length.getPx();
    }
    CSSAutoLengthValueImp() :
        length(0) {
    }
    CSSAutoLengthValueImp(float number, unsigned short unit) :
        length(number, unit) {
    }
    CSSAutoLengthValueImp& operator=(const CSSAutoLengthValueImp& other) {
        length = other.length;
        return *this;
    }
    static const char16_t* Options[];
};

// <length>, <percentage>, none
// Negative values are illegal.
class CSSNoneLengthValueImp : public CSSPropertyValueImp
{
protected:
    CSSNumericValue length;
public:
    CSSNoneLengthValueImp& setValue(float number, unsigned short unit) {
        length.setValue(number, unit);
        if (length.isNegative())
            length.setIndex(0);
        return *this;
    }
    CSSNoneLengthValueImp& setValue(CSSParserTerm* term = 0) {
        if (!term)
            length.setIndex(0);
        else {
            length.setValue(term);
            if (length.isNegative())
                length.setIndex(0);
        }
        return *this;
    }
    bool isNone() const {
        return length.getIndex() == 0;
    }
    bool isPercentage() const {
        return length.isPercentage();
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (isNone())
            return u"none";
        return length.getCssText();
    }
    bool operator==(const CSSNoneLengthValueImp& value) {
        return length == value.length;
    }
    bool operator!=(const CSSNoneLengthValueImp& value) {
        return length != value.length;
    }
    void specify(const CSSNoneLengthValueImp& specified) {
        length.specify(specified.length);
    }
    void inherit(const CSSNoneLengthValueImp& parent) {
        length.inheritLength(parent.length);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, CSSStyleDeclarationImp* style, float fullSize);
    float getPx() const {
        return length.getPx();
    }
    CSSNoneLengthValueImp() :
        length(0) {
    }
    CSSNoneLengthValueImp(float number, unsigned short unit) :
        length(number, unit) {
    }
};

// <length>, normal
class CSSNormalLengthValueImp : public CSSPropertyValueImp
{
protected:
    CSSNumericValue length;
public:
    enum {
         Normal
    };
    CSSNormalLengthValueImp& setValue(float number, unsigned short unit) {
        length.setValue(number, unit);
        return *this;
    }
    CSSNormalLengthValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            length.setValue(term);
        else
            length.setIndex(Normal);
        return *this;
    }
    bool isNormal() const {
        return length.getIndex() == Normal;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return length.getCssText(Options);
    }
    bool operator==(const CSSNormalLengthValueImp& value) {
        return length == value.length;
    }
    bool operator!=(const CSSNormalLengthValueImp& value) {
        return length != value.length;
    }
    void specify(const CSSNormalLengthValueImp& specified) {
        length.specify(specified.length);
    }
    void inherit(const CSSNormalLengthValueImp& parent) {
        length.inheritLength(parent.length);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    float getPx() const {
        return length.getPx();
    }
    CSSNormalLengthValueImp() :
        length(Normal)
    {}
    CSSNormalLengthValueImp(float number, unsigned short unit) :
        length(number, unit)
    {}
    static const char16_t* Options[];
};

class CSSLetterSpacingValueImp : public CSSNormalLengthValueImp
{
public:
    CSSLetterSpacingValueImp() {}
    CSSLetterSpacingValueImp(float number, unsigned short unit) :
        CSSNormalLengthValueImp(number, unit)
    {}
};

class CSSWordSpacingValueImp : public CSSNormalLengthValueImp
{
public:
    CSSWordSpacingValueImp() {}
    CSSWordSpacingValueImp(float number, unsigned short unit) :
        CSSNormalLengthValueImp(number, unit)
    {}
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void inherit(const CSSWordSpacingValueImp& parent) {
        length.inherit(parent.length);
    }
};

class CounterImp;

class CSSAutoNumberingValueImp : public CSSPropertyValueImp
{
public:
    struct Content {
        std::u16string name;
        int number;
        Content(const std::u16string& name, int number) :
            name(name),
            number(number) {
        }
        std::u16string getCssText(int defaultNumber) const {
            if (number == defaultNumber)
                return CSSSerializeIdentifier(name);
            return CSSSerializeIdentifier(name) + u' ' + CSSSerializeInteger(number);
        }
        Content* clone() {
            return new(std::nothrow) Content(name, number);
        }
    };

    struct CounterContext
    {
        ViewCSSImp* view;
        std::list<CounterImp*> counters;
    public:
        CounterContext(ViewCSSImp* view) :
            view(view)
        {}
        ~CounterContext();
        bool hasCounter() const {
            return !counters.empty();
        }
        bool hasCounter(const std::u16string& name) const;
        void addCounter(CounterImp* counter) {
            counters.push_front(counter);
        }
    };


private:
    std::list<Content*> contents;
    int defaultNumber;

    void clearContents() {
        while (!contents.empty()) {
            Content* content = contents.front();
            delete content;
            contents.pop_front();
        }
    }

public:
    virtual ~CSSAutoNumberingValueImp() {
        clearContents();
    }
    void reset() {
        clearContents();
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl = 0) const;
    bool operator==(const CSSAutoNumberingValueImp& other);
    bool operator!=(const CSSAutoNumberingValueImp& other) {
        return !(*this == other);
    }
    void specify(const CSSAutoNumberingValueImp& specified) {
        reset();
        for (auto i = specified.contents.begin(); i != specified.contents.end(); ++i) {
            if (Content* clone = (*i)->clone())
                contents.push_back(clone);
        }
    }
    bool hasCounter() const {
        return !contents.empty();
    }
    void incrementCounter(ViewCSSImp* view, CounterContext* context);
    void resetCounter(ViewCSSImp* view, CounterContext* context);

    CSSAutoNumberingValueImp(int defaultNumber) :
        defaultNumber(defaultNumber) {
    }
};

class CSSBackgroundAttachmentValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Scroll,
        Fixed
    };
    CSSBackgroundAttachmentValueImp& setValue(unsigned value = Scroll) {
        this->value = value;
        return *this;
    }
    CSSBackgroundAttachmentValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    bool isFixed() const {
        return value == Fixed;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    void specify(const CSSBackgroundAttachmentValueImp& specified) {
        value = specified.value;
    }
    CSSBackgroundAttachmentValueImp(unsigned initial = Scroll) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSBackgroundImageValueImp : public CSSPropertyValueImp
{
    std::u16string uri;
public:
    enum {
        None
    };
    CSSBackgroundImageValueImp& setValue(const std::u16string uri = u"") {
        this->uri = uri;
        return *this;
    }
    CSSBackgroundImageValueImp& setValue(CSSParserTerm* term) {
        if (0 <= term->getIndex())
            return setValue();
        else
            return setValue(term->getURL());
    }
    std::u16string getValue() const {
        return uri;
    }
    bool isNone() const {
        return uri.length() == 0;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (isNone())
            return u"none";
        return u"url(" + CSSSerializeString(uri) + u')';
    }
    void specify(const CSSBackgroundImageValueImp& specified) {
        uri = specified.uri;
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* self);
    CSSBackgroundImageValueImp() {
    }
};

class CSSBackgroundPositionValueImp : public CSSPropertyValueImp
{
    CSSNumericValue horizontal;
    CSSNumericValue vertical;
public:
    enum {
        Top,
        Right,
        Bottom,
        Left,
        Center
    };
    CSSBackgroundPositionValueImp& setValue(float h = 0.0f, short horizontalUnit = css::CSSPrimitiveValue::CSS_PERCENTAGE,
                                            float v = 0.0f, short verticalUnit = css::CSSPrimitiveValue::CSS_PERCENTAGE) {
        horizontal.setValue(h, horizontalUnit);
        vertical.setValue(v, verticalUnit);
        return *this;
    }
    std::deque<CSSParserTerm*>::iterator setValue(std::deque<CSSParserTerm*>& stack, std::deque<CSSParserTerm*>::iterator i);
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return horizontal.getCssText() + u' ' + vertical.getCssText();
    }
    void specify(const CSSBackgroundPositionValueImp& specified) {
        horizontal.specify(specified.horizontal);
        vertical.specify(specified.vertical);
    }
    void inherit(const CSSBackgroundPositionValueImp& parent) {
        horizontal.inheritLength(parent.horizontal);
        vertical.inheritLength(parent.vertical);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, BoxImage* image, CSSStyleDeclarationImp* style, float width, float height);
    float getLeftPx() const {
        return horizontal.getPx();
    }
    float getTopPx() const {
        return vertical.getPx();
    }
    CSSBackgroundPositionValueImp() :
        horizontal(0.0, css::CSSPrimitiveValue::CSS_PERCENTAGE),
        vertical(0.0, css::CSSPrimitiveValue::CSS_PERCENTAGE) {
    }
};

class CSSBackgroundRepeatValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        NoRepeat,
        RepeatX,
        RepeatY,
        Repeat,
    };
    CSSBackgroundRepeatValueImp& setValue(unsigned value = Repeat) {
        this->value = value;
        return *this;
    }
    CSSBackgroundRepeatValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    void specify(const CSSBackgroundRepeatValueImp& specified) {
        value = specified.value;
    }
    CSSBackgroundRepeatValueImp (unsigned initial = Repeat) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSBackgroundShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    void reset(CSSStyleDeclarationImp* self);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSBorderColorValueImp : public CSSPropertyValueImp
{
    unsigned value;
    bool hasValue;
    unsigned resolved;
public:
    CSSBorderColorValueImp& setValue(unsigned color = 0xff000000) {
        resolved = value = color;
        hasValue = true;
        return *this;
    }
    CSSBorderColorValueImp& setValue(CSSParserTerm* term) {
        assert(term->unit == css::CSSPrimitiveValue::CSS_RGBCOLOR);
        hasValue = true;
        return setValue(term->rgb);
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return CSSSerializeRGB(value);
    }
    bool operator==(const CSSBorderColorValueImp& color) const {
        if (!hasValue && !color.hasValue)
            return true;
        if (hasValue != color.hasValue)
            return false;
        return value == color.value;
    }
    bool operator!=(const CSSBorderColorValueImp& color) const {
        return !(*this == color);
    }
    void specify(const CSSBorderColorValueImp& specified) {
        value = specified.value;
        hasValue = specified.hasValue;
        resolved = specified.resolved;
    }
    CSSBorderColorValueImp& reset() {
        resolved = value = 0xff000000;
        hasValue = false;
        return *this;
    }
    unsigned getARGB() const {
        return resolved;
    }
    void setARGB(unsigned argb) {
        resolved = argb;
    }
    void compute(CSSStyleDeclarationImp* decl);
    CSSBorderColorValueImp(unsigned argb = 0xff000000) :
        value(0xff000000),
        hasValue(false)
    {}
};

class CSSBorderSpacingValueImp : public CSSPropertyValueImp
{
    CSSNumericValue horizontal;
    CSSNumericValue vertical;
public:
    CSSBorderSpacingValueImp& setValue(float h = 0.0f, short horizontalUnit = css::CSSPrimitiveValue::CSS_PX,
                                       float v = 0.0f, short verticalUnit = css::CSSPrimitiveValue::CSS_PX) {
        horizontal.setValue(h, horizontalUnit);
        vertical.setValue(v, verticalUnit);
        return *this;
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser) {
        std::deque<CSSParserTerm*>& stack = parser->getStack();
        if (stack.size() == 1)
            setValue(stack[0]->number, stack[0]->unit, stack[0]->number, stack[0]->unit);
        else
            setValue(stack[0]->number, stack[0]->unit, stack[1]->number, stack[1]->unit);
        return true;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return horizontal.getCssText() + u' ' + vertical.getCssText();
    }
    bool operator==(const CSSBorderSpacingValueImp& borderSpacing) const {
        return horizontal == borderSpacing.horizontal && vertical == borderSpacing.vertical;
    }
    bool operator!=(const CSSBorderSpacingValueImp& borderSpacing) const {
        return horizontal != borderSpacing.horizontal || vertical != borderSpacing.vertical;
    }
    void specify(const CSSBorderSpacingValueImp& specified) {
        horizontal.specify(specified.horizontal);
        vertical.specify(specified.vertical);
    }
    void inherit(const CSSBorderSpacingValueImp& parent) {
        horizontal.inheritLength(parent.horizontal);
        vertical.inheritLength(parent.vertical);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    float getHorizontalSpacing() const {
        return horizontal.getPx();
    }
    float getVerticalSpacing() const {
        return vertical.getPx();
    }
    CSSBorderSpacingValueImp() :
        horizontal(0, css::CSSPrimitiveValue::CSS_PX),
        vertical(0, css::CSSPrimitiveValue::CSS_PX) {
    }
};

class CSSBorderCollapseValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Collapse,
        Separate
    };
    CSSBorderCollapseValueImp& setValue(unsigned value = Separate) {
        this->value = value;
        return *this;
    }
    CSSBorderCollapseValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSBorderCollapseValueImp& borderCollapse) const {
        return value == borderCollapse.value;
    }
    bool operator!=(const CSSBorderCollapseValueImp& borderCollapse) const {
        return value != borderCollapse.value;
    }
    void specify(const CSSBorderCollapseValueImp& specified) {
        value = specified.value;
    }
    CSSBorderCollapseValueImp(unsigned initial = Separate) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSBorderColorShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSBorderStyleValueImp : public CSSPropertyValueImp
{
    unsigned value;
    unsigned resolved;
public:
    enum {
        // In the preferred order for the border conflict resolution
        None,
        Inset,
        Groove,
        Outset,
        Ridge,
        Dotted,
        Dashed,
        Solid,
        Double,
        Hidden
    };
    CSSBorderStyleValueImp& setValue(unsigned value = None) {
        resolved = this->value = value;
        return *this;
    }
    CSSBorderStyleValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return resolved;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[resolved];
    }
    bool operator==(const CSSBorderStyleValueImp& style) const {
        return resolved == style.resolved;
    }
    bool operator!=(const CSSBorderStyleValueImp& style) const {
        return resolved != style.resolved;
    }
    bool operator<(const CSSBorderStyleValueImp& style) const {
        return resolved < style.resolved;
    }
    void specify(const CSSBorderStyleValueImp& specified) {
        value = specified.value;
        resolved = specified.resolved;
    }
    void compute() {
        resolved = value;
    }
    CSSBorderStyleValueImp(unsigned initial = None) :
        value(initial),
        resolved(initial)
    {}
    static const char16_t* Options[];
};

class CSSBorderStyleShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSBorderWidthValueImp : public CSSPropertyValueImp
{
    CSSNumericValue width;
public:
    enum {
         Thin,
         Medium,
         Thick
    };
    CSSBorderWidthValueImp& setValue(float number, unsigned short unit) {
        width.setValue(number, unit);
        return *this;
    }
    CSSBorderWidthValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            width.setValue(term);
        else
            width.setIndex(Medium);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return width.getCssText(Options);
    }
    bool operator==(const CSSBorderWidthValueImp& value) const {
        return width == value.width;
    }
    bool operator!=(const CSSBorderWidthValueImp& value) const {
        return width != value.width;
    }
    bool operator<(const CSSBorderWidthValueImp& value) const {
        return getPx() < value.getPx();
    }
    void specify(const CSSBorderWidthValueImp& specified) {
        width.specify(specified.width);
    }
    void inherit(const CSSBorderWidthValueImp& parent) {
        width.inherit(parent.width);
    }
    void compute(ViewCSSImp* view, const CSSBorderStyleValueImp& borderStyle, CSSStyleDeclarationImp* style);
    float getPx() const {
        return width.getPx();
    }
    CSSBorderWidthValueImp() :
        width(Medium)
    {}
    CSSBorderWidthValueImp(float number) :
        width(number)
    {}
    static const char16_t* Options[];
};

class CSSBorderWidthShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSBorderValueImp : public CSSPropertyValueImp
{
    unsigned index;
public:
    CSSBorderValueImp(unsigned index) :
        index(index)
    {
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSBorderShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSCaptionSideValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Top,
        Bottom
    };
    CSSCaptionSideValueImp& setValue(unsigned value = Top) {
        this->value = value;
        return *this;
    }
    CSSCaptionSideValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSCaptionSideValueImp& captionSide) const {
        return value == captionSide.value;
    }
    bool operator!=(const CSSCaptionSideValueImp& captionSide) const {
        return value != captionSide.value;
    }
    void specify(const CSSCaptionSideValueImp& specified) {
        value = specified.value;
    }
    CSSCaptionSideValueImp(unsigned initial = Top) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSClearValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        None,
        Left,
        Right,
        Both
    };
    CSSClearValueImp& setValue(unsigned value = None) {
        this->value = value;
        return *this;
    }
    CSSClearValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSClearValueImp& clear) const {
        return value == clear.value;
    }
    bool operator!=(const CSSClearValueImp& clear) const {
        return value != clear.value;
    }
    void specify(const CSSClearValueImp& specified) {
        value = specified.value;
    }
    CSSClearValueImp(unsigned initial = None) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSColorValueImp : public CSSPropertyValueImp
{
    unsigned value;
    unsigned resolved;
public:
    static const unsigned Transparent = 0x00000000u;
    CSSColorValueImp& setValue(unsigned color = 0xff000000) {
        resolved = value = color;
        return *this;
    }
    CSSColorValueImp& setValue(CSSParserTerm* term) {
        assert(term->unit == css::CSSPrimitiveValue::CSS_RGBCOLOR);
        return setValue(term->rgb);
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return CSSSerializeRGB(resolved);
    }
    bool operator==(const CSSColorValueImp& color) const {
        return value == color.value;
    }
    bool operator!=(const CSSColorValueImp& color) const {
        return value != color.value;
    }
    void specify(const CSSColorValueImp& specified) {
        value = specified.value;
        resolved = specified.resolved;
    }
    void compute() {
        resolved = value;
    }
    unsigned getARGB() const {
        return resolved;
    }
    void setARGB(unsigned argb) {
        resolved = argb;
    }
    CSSColorValueImp(unsigned argb = 0xff000000) :
        value(argb),
        resolved(argb)
    {}
};

class CSSContentValueImp : public CSSPropertyValueImp
{
    typedef CSSAutoNumberingValueImp::CounterContext CounterContext;

public:
    enum {
        Normal,
        None,
        // QuoteContent
        OpenQuote,
        CloseQuote,
        NoOpenQuote,
        NoCloseQuote,
        // Attr
        Attr,
        Counter // pseudo
    };
    struct Content {
        virtual ~Content() {}
        virtual std::u16string getCssText() const = 0;
        virtual std::u16string eval(ViewCSSImp* view, Element element, CounterContext* context) {
            return u"";
        }
        virtual Content* clone() = 0;
    };
    struct StringContent : public Content {
        std::u16string value;
        StringContent(const std::u16string& value) :
            value(value) {
        }
        virtual std::u16string getCssText() const {
            return CSSSerializeString(value);
        }
        virtual std::u16string eval(ViewCSSImp* view, Element element, CounterContext* context) {
            return value;
        }
        virtual StringContent* clone() {
            return new(std::nothrow) StringContent(value);
        }
    };
    struct URIContent : public Content {
        std::u16string value;
        URIContent(const std::u16string& value) :
            value(value) {
        }
        virtual std::u16string getCssText() const {
            return u"url(" + CSSSerializeString(value) + u')';
        }
        virtual URIContent* clone() {
            return new(std::nothrow) URIContent(value);
        }
    };
    struct CounterContent : public Content {
        std::u16string identifier;
        std::u16string string;
        bool nested;
        CSSListStyleTypeValueImp listStyleType;
        CounterContent(const std::u16string& identifier, unsigned listStyleType) :
            identifier(identifier),
            nested(false),
            listStyleType(listStyleType) {
        }
        CounterContent(const std::u16string& identifier, const std::u16string& string, unsigned listStyleType) :
            identifier(identifier),
            string(string),
            nested(true),
            listStyleType(listStyleType) {
        }
        virtual std::u16string getCssText() const {
            if (nested)
                return u"counters(" +  CSSSerializeIdentifier(identifier) + u", " + CSSSerializeString(string) + u", " + listStyleType.getCssText() + u')';
            return u"counter(" +  CSSSerializeIdentifier(identifier) + u", " + listStyleType.getCssText() + u')';
        }
        virtual std::u16string eval(ViewCSSImp* view, Element element, CounterContext* context);
        virtual CounterContent* clone() {
            if (nested)
                return new(std::nothrow) CounterContent(identifier, string, listStyleType.getValue());
            else
                return new(std::nothrow) CounterContent(identifier, listStyleType.getValue());
        }
    };
    struct AttrContent : public Content {
        std::u16string identifier;
        AttrContent(const std::u16string& value) :
            identifier(value) {
        }
        virtual std::u16string getCssText() const {
            return u"attr(" + CSSSerializeIdentifier(identifier) + u')';
        }
        virtual std::u16string eval(ViewCSSImp* view, Element element, CounterContext* context);
        virtual AttrContent* clone() {
            return new(std::nothrow) AttrContent(identifier);
        }
    };
    struct QuoteContent : public Content {
        unsigned value;
        QuoteContent(unsigned value) :
            value(value) {
        }
        virtual std::u16string getCssText() const {
            return CSSContentValueImp::Options[value];
        }
        virtual std::u16string eval(ViewCSSImp* view, Element element, CounterContext* context);
        virtual QuoteContent* clone() {
            return new(std::nothrow) QuoteContent(value);
        }
    };

    void clearContents() {
        while (!contents.empty()) {
            Content* content = contents.front();
            delete content;
            contents.pop_front();
        }
    }

protected:
    unsigned original;
    unsigned value;  // Normal or None; ignore this value if contents is not empty.
    std::list<Content*> contents;

public:
    ~CSSContentValueImp() {
        clearContents();
    }
    void reset() {
        original = value = Normal;
        clearContents();
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);

    bool isNone() const {
        return contents.empty() && value == None;
    }
    bool isNormal() const {
        return contents.empty() && value == Normal;
    }
    bool wasNormal() const {
        return original == Normal;
    }

    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl = 0) const;
    bool operator==(const CSSContentValueImp& content);
    bool operator!=(const CSSContentValueImp& content) {
        return !(*this == content);
    }
    void specify(const CSSContentValueImp& specified);
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    std::u16string evalText(ViewCSSImp* view, Element element, CounterContext* context);
    Element eval(ViewCSSImp* view, Element element, CounterContext* context);

    CSSContentValueImp(unsigned initial = Normal) :
        original(initial),
        value(initial)
    {
    }
    static const char16_t* Options[];
};

class CSSCursorValueImp : public CSSPropertyValueImp
{
    std::list<std::u16string> uriList;
    unsigned value;
public:
    enum {
        Auto,
        Crosshair,
        Default,
        Pointer,
        Move,
        EResize,
        NEResize,
        NWResize,
        NResize,
        SEResize,
        SWResize,
        SResize,
        WResize,
        Text,
        Wait,
        Help,
        Progress,

        CursorsMax
    };
    unsigned getValue() const {
        return value;
    }
    void reset() {
        uriList.clear();
        value = Auto;
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(const CSSCursorValueImp& specified) {
        uriList = specified.uriList;
        value = specified.value;
    }
    CSSCursorValueImp() :
        value(Auto)
    {
    }
    static const char16_t* Options[];
};

class CSSDirectionValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Ltr,
        Rtl
    };
    CSSDirectionValueImp& setValue(unsigned value = Ltr) {
        this->value = value;
        return *this;
    }
    CSSDirectionValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSDirectionValueImp& direction) {
        return value == direction.value;
    }
    bool operator!=(const CSSDirectionValueImp& direction) {
        return value != direction.value;
    }
    void specify(const CSSDirectionValueImp& specified) {
        value = specified.value;
    }
    CSSDirectionValueImp(unsigned initial = Ltr) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSDisplayValueImp : public CSSPropertyValueImp
{
    unsigned original;  // retains the original value for calculating the static position of an absolutely positioned box.
    unsigned value;
public:
    enum {
        Inline,
        Block,
        ListItem,
        RunIn,
        InlineBlock,
        Table,
        InlineTable,
        TableRowGroup,
        TableHeaderGroup,
        TableFooterGroup,
        TableRow,
        TableColumnGroup,
        TableColumn,
        TableCell,
        TableCaption,
        None
    };
    CSSDisplayValueImp& setValue(unsigned value = Inline) {
        original = this->value = value;
        return *this;
    }
    CSSDisplayValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    unsigned getOriginalValue() const {
        return original;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSDisplayValueImp& display) const {
        return original == display.original;
    }
    bool operator!=(const CSSDisplayValueImp& display) const {
        return original != display.original;
    }
    void specify(const CSSDisplayValueImp& specified) {
        original = value = specified.original;
    }
    void compute(CSSStyleDeclarationImp* decl, Element element);
    CSSDisplayValueImp(unsigned initial = Inline) :
        original(initial),
        value(initial)
    {
    }
    bool isBlockLevel() const {
        return isBlockLevel(value);
    }
    bool isInlineBlock() const {
        switch (value) {
        case InlineBlock:
        case InlineTable:
            return true;
        default:
            return false;
        }
    }
    bool isInlineLevel() const {
        switch (value) {
        case Inline:
        case InlineBlock:
        case InlineTable:
        // Ruby:
            return true;
        default:
            return false;
        }
    }
    bool isInline() const {
        return value == Inline;
    }
    bool isRunIn() const {
        return value == RunIn;
    }
    bool isNone() const {
        return value == None;
    }
    bool isListItem() const {
        return value == ListItem;
    }
    bool isTableParts() const {
        return isTableParts(value);
    }
    bool isRowGoupBox() const {
        switch (value) {
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
            return true;
        default:
            return false;
        }
    }
    bool isProperTableChild() const {
        return isProperTableChild(value);
    }
    bool isProperTableRowParent() const {
        switch (value) {
        case Table:
        case InlineTable:
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
            return true;
        default:
            return false;
        }
    }
    bool isInternalTableBox() const {
        switch (value) {
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
        case TableRow:
        case TableColumnGroup:
        case TableColumn:
        case TableCell:
            return true;
        default:
            return false;
        }
    }
    bool isTabularContainer() const {
        switch (value) {
        case Table:
        case InlineTable:
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
        case TableRow:
            return true;
        default:
            return false;
        }
    }
    bool isFlowRoot() const {
        switch (value) {
        case TableCell:
        case TableCaption:
        case InlineBlock:
        case InlineTable:
            return true;
        default:
            return false;
        }
    }

    static const char16_t* Options[];

    static bool isBlockLevel(unsigned value) {
        switch (value) {
        case Block:
        case ListItem:
        case Table:
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
        case TableRow:
        case TableColumnGroup:
        case TableColumn:
        case TableCell:
        case TableCaption:
        // <template>:
            return true;
        default:
            return false;
        }
    }
    static bool isTableParts(unsigned value) {
        switch (value) {
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
        case TableRow:
        case TableColumnGroup:
        case TableColumn:
        case TableCell:
        case TableCaption:
            return true;
        default:
            return false;
        }
    }
    static bool isProperTableChild(unsigned value) {
        switch (value) {
        case TableRowGroup:
        case TableHeaderGroup:
        case TableFooterGroup:
        case TableRow:
        case TableColumnGroup:
        case TableColumn:
        case TableCaption:
            return true;
        default:
            return false;
        }
    }
};

class CSSEmptyCellsValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Hide,
        Show
    };
    CSSEmptyCellsValueImp& setValue(unsigned value = Show) {
        this->value = value;
        return *this;
    }
    CSSEmptyCellsValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    void specify(const CSSEmptyCellsValueImp& specified) {
        value = specified.value;
    }
    CSSEmptyCellsValueImp(unsigned initial = Show) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSFloatValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        None,
        Left,
        Right
    };
    CSSFloatValueImp& setValue(unsigned value = None) {
        this->value = value;
        return *this;
    }
    virtual CSSFloatValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSFloatValueImp& n) {
        return value == n.value;
    }
    bool operator!=(const CSSFloatValueImp& n) {
        return value != n.value;
    }
    void specify(const CSSFloatValueImp& specified) {
        value = specified.value;
    }
    CSSFloatValueImp(unsigned initial = None) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSFontFamilyValueImp : public CSSPropertyValueImp
{
    unsigned generic;
    std::list<std::u16string> familyNames;
public:
    enum {
        None,
        Serif,
        SansSerif,
        Cursive,
        Fantasy,
        Monospace
    };
    void reset() {
        generic = None;
        familyNames.clear();
    }
    void setGeneric(unsigned generic) {
        this->generic = generic;
    }
    void addFamily(const std::u16string name) {
        familyNames.push_back(name);
    }
    std::deque<CSSParserTerm*>::iterator setValue(std::deque<CSSParserTerm*>& stack, std::deque<CSSParserTerm*>::iterator i);
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const ;
    bool operator==(const CSSFontFamilyValueImp& value) {
        if (generic != value.generic)
            return false;
        if (familyNames.size() != value.familyNames.size())
            return false;
        auto j = value.familyNames.begin();
        for (auto i = familyNames.begin(); i != familyNames.end(); ++i, ++j) {
            if (*i != *j)
                return false;
        }
        return true;
    }
    bool operator!=(const CSSFontFamilyValueImp& value) {
        return !(*this == value);
    }
    void specify(const CSSFontFamilyValueImp& specified) {
        generic = specified.generic;
        familyNames = specified.familyNames;
    }
    unsigned getGeneric() const {
        return generic;
    }
    const std::list<std::u16string>& getFamilyNames() const {
        return familyNames;
    }
    CSSFontFamilyValueImp() :
        generic(None) {
    }
    static const char16_t* Options[];
};

class CSSFontSizeValueImp : public CSSPropertyValueImp
{
    CSSNumericValue size;
public:
    enum {
        XxSmall,
        XSmall,
        Small,
        Medium,
        Large,
        XLarge,
        XxLarge,
        Larger,
        Smaller
    };
    CSSFontSizeValueImp& setValue(float number, unsigned short unit) {
        size.setValue(number, unit);
        return *this;
    }
    virtual CSSFontSizeValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            size.setValue(term);
        else
            size.setIndex(Medium);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return size.getResolvedCssText(Options);
    }
    bool operator==(const CSSFontSizeValueImp& fontSize) {
        return size == fontSize.size;
    }
    bool operator!=(const CSSFontSizeValueImp& fontSize) {
        return size != fontSize.size;
    }
    void specify(const CSSFontSizeValueImp& specified) {
        size.specify(specified.size);
    }
    void inherit(const CSSFontSizeValueImp& parent) {
        size.inherit(parent.size);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* parentStyle);
    float getPx() const {
        return size.getPx();
    }
    CSSFontSizeValueImp() :
        size(Medium) {
    }
    static const char16_t* Options[];
};

class CSSFontStyleValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Normal,
        Italic,
        Oblique
    };
    CSSFontStyleValueImp& setValue(unsigned value = Normal) {
        this->value = value;
        return *this;
    }
    CSSFontStyleValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSFontStyleValueImp& fontStyle) {
        return value == fontStyle.value;
    }
    bool operator!=(const CSSFontStyleValueImp& fontStyle) {
        return value != fontStyle.value;
    }
    void specify(const CSSFontStyleValueImp& specified) {
        value = specified.value;
    }
    unsigned getStyle() const {
        return value;
    }
    bool isNormal() const {
        return value == Normal;
    }
    CSSFontStyleValueImp(unsigned initial = Normal) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSFontVariantValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Normal,
        SmallCaps
    };
    CSSFontVariantValueImp& setValue(unsigned value = Normal) {
        this->value = value;
        return *this;
    }
    CSSFontVariantValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSFontVariantValueImp& fontVariant) {
        return value == fontVariant.value;
    }
    bool operator!=(const CSSFontVariantValueImp& fontVariant) {
        return value != fontVariant.value;
    }
    void specify(const CSSFontVariantValueImp& specified) {
        value = specified.value;
    }
    unsigned getValue() const {
        return value;
    }
    bool isNormal() const {
        return value == Normal;
    }
    CSSFontVariantValueImp(unsigned initial = Normal) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSFontWeightValueImp : public CSSPropertyValueImp
{
    CSSNumericValue value;
public:
    enum {
        Normal,
        Bold,
        Bolder,
        Lighter
    };
    CSSFontWeightValueImp& setValue(unsigned weight) {
        value.setValue(static_cast<float>(weight), css::CSSPrimitiveValue::CSS_NUMBER);
        return *this;
    }
    CSSFontWeightValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            value.setValue(term);
        else
            value.setIndex(Normal);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return value.getResolvedCssText(Options, css::CSSPrimitiveValue::CSS_NUMBER);
    }
    bool operator==(const CSSFontWeightValueImp& fontWeight) {
        return value == fontWeight.value;
    }
    bool operator!=(const CSSFontWeightValueImp& fontWeight) {
        return value != fontWeight.value;
    }
    void specify(const CSSFontWeightValueImp& specified) {
        value.specify(specified.value);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* parentStyle);
    unsigned getWeight() const {
        return static_cast<unsigned>(value.getPx());
    }
    bool isNormal() const {
        if (value.getIndex() == Normal)
            return true;
        if (value.unit == css::CSSPrimitiveValue::CSS_NUMBER && value.number == 400.0f)
            return true;
        return false;
    }
    CSSFontWeightValueImp() :
        value(Normal)
    {}
    static const char16_t* Options[];
};

class CSSFontShorthandImp : public CSSPropertyValueImp
{
    unsigned index;
public:
    enum {
        Normal,
        Caption,
        Icon,
        Menu,
        MessageBox,
        SmallCaption,
        StatusBar
    };
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    void reset(CSSStyleDeclarationImp* self);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
    CSSFontShorthandImp() :
        index(Normal) {
    }
    static const char16_t* Options[];
};

// normal | <number> | <length> | <percentage>
class CSSLineHeightValueImp : public CSSPropertyValueImp
{
    CSSNumericValue value;
public:
    enum {
        Normal
    };
    CSSLineHeightValueImp& setValue(float number, unsigned short unit) {
        value.setValue(number, unit);
        return *this;
    }
    CSSLineHeightValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            value.setValue(term);
        else
            value.setIndex(Normal);
        return *this;
    }
    bool isNormal() const {
        return value.getIndex() == Normal;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return value.getResolvedCssText(Options);
    }
    bool operator==(const CSSLineHeightValueImp& lineHeight) {
        return value == lineHeight.value;
    }
    bool operator!=(const CSSLineHeightValueImp& lineHeight) {
        return value != lineHeight.value;
    }
    void specify(const CSSLineHeightValueImp& specified) {
        value.specify(specified.value);
    }
    void inherit(const CSSLineHeightValueImp& parent);
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    float getPx() const {
        return value.getPx();
    }
    CSSLineHeightValueImp() :
        value(Normal)
    {}
    static const char16_t* Options[];
};

class CSSListStyleImageValueImp : public CSSPropertyValueImp
{
    std::u16string uri;
    HttpRequest* request;
    unsigned requestID;
    unsigned status;

    void notify(CSSStyleDeclarationImp* self);

public:
    enum {
        None
    };
    CSSListStyleImageValueImp& setValue(const std::u16string uri = u"") {
        this->uri = uri;
        return *this;
    }
    CSSListStyleImageValueImp& setValue(CSSParserTerm* term) {
        if (0 <= term->getIndex())
            return setValue();
        else
            return setValue(term->getURL());
    }
    std::u16string getValue() const {
        return uri;
    }
    bool isNone() const {
        return uri.length() == 0;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (isNone())
            return u"none";
        return u"url(" + CSSSerializeString(uri) + u')';
    }
    bool operator==(const CSSListStyleImageValueImp& image) const {
        return uri == image.uri && status == image.status;
    }
    bool operator!=(const CSSListStyleImageValueImp& image) const {
        return uri != image.uri || status != image.status;
    }
    void specify(const CSSListStyleImageValueImp& specified) {
        uri = specified.uri;
        status = specified.status;
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* self);
    bool hasImage() const {
        return status == 200;   // TODO: check content as well
    }
    CSSListStyleImageValueImp() :
        request(0),
        requestID(static_cast<unsigned>(-1)),
        status(0)
    {}
     ~CSSListStyleImageValueImp() {
        if (request)
            request->clearCallback(requestID);
    }
};

class CSSListStylePositionValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Inside,
        Outside,
    };
    unsigned getValue() const {
        return value;
    }
    CSSListStylePositionValueImp& setValue(unsigned value = Outside) {
        this->value = value;
        return *this;
    }
    CSSListStylePositionValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSListStylePositionValueImp& position) const {
        return value == position.value;
    }
    bool operator!=(const CSSListStylePositionValueImp& position) const {
        return value != position.value;
    }
    void specify(const CSSListStylePositionValueImp& specified) {
        value = specified.value;
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    CSSListStylePositionValueImp(unsigned initial = Outside) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSListStyleShorthandImp : public CSSPropertyValueImp
{
public:
    enum {
        None
    };
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    void reset(CSSStyleDeclarationImp* self);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSOverflowValueImp : public CSSPropertyValueImp
{
    unsigned original;
    unsigned value;
public:
    enum {
        Visible,
        Hidden,
        Scroll,
        Auto
    };
    CSSOverflowValueImp& setValue(unsigned value = Visible) {
        original = this->value = value;
        return *this;
    }
    CSSOverflowValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    unsigned getOriginalValue() const {
        return original;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool isClipped() const {
        return value != Visible;
    }
    bool canScroll() const {
        return canScroll(value);
    }
    bool operator==(const CSSOverflowValueImp& overflow) {
        return original == overflow.original;
    }
    bool operator!=(const CSSOverflowValueImp& overflow) {
        return original != overflow.original;
    }
    void specify(const CSSOverflowValueImp& specified) {
        original = value = specified.original;
    }
    void useBodyValue(CSSOverflowValueImp& body) {
        value = body.original;
        body.value = CSSOverflowValueImp::Visible;
        if (value == Visible)   // cf. http://www.w3.org/TR/CSS21/visufx.html#propdef-overflow
            value = Auto;
    }
    CSSOverflowValueImp(unsigned initial = Visible) :
        original(initial),
        value(initial)
    {}
    static const char16_t* Options[];

    static bool canScroll(unsigned value) {
        return value == Scroll || value == Auto;
    }
};

class CSSPageBreakValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Auto,
        Always,
        Avoid,
        Left,
        Right
    };
    CSSPageBreakValueImp& setValue(unsigned value = Auto) {
        this->value = value;
        return *this;
    }
    CSSPageBreakValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    void specify(const CSSPageBreakValueImp& specified) {
        value = specified.value;
    }
    CSSPageBreakValueImp(unsigned initial = Auto) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSMarginShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSOutlineColorValueImp : public CSSPropertyValueImp
{
    int invert;
    unsigned color;
    int resolvedInvert;
    unsigned resolvedColor;
public:
    enum {
        Color = 0,
        Invert = 1,
    };
    CSSOutlineColorValueImp& setValue(int index = 1, unsigned argb = 0xff000000) {
        resolvedInvert = invert = index;
        resolvedColor = color = argb;
        return *this;
    }
    CSSOutlineColorValueImp& setValue(CSSParserTerm* term) {
        if (0 <= term->getIndex())
            return setValue(term->getIndex());
        else {
            assert(term->unit == css::CSSPrimitiveValue::CSS_RGBCOLOR);
            return setValue(0, term->rgb);
        }
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return resolvedInvert ? u"invert" : CSSSerializeRGB(resolvedColor);
    }
    bool operator==(const CSSOutlineColorValueImp& value) const {
        return (resolvedInvert == value.resolvedInvert) && (resolvedInvert == Invert || resolvedColor == value.resolvedColor);
    }
    bool operator!=(const CSSOutlineColorValueImp& value) const {
        return (resolvedInvert != value.resolvedInvert) || (resolvedInvert == Color && resolvedColor != value.resolvedColor);
    }
    void specify(const CSSOutlineColorValueImp& specified) {
        invert = specified.invert;
        color = specified.color;
        resolvedInvert = specified.resolvedInvert;
        resolvedColor = specified.resolvedColor;
    }
    void copy(const CSSOutlineColorValueImp& specified) {
        resolvedInvert = specified.resolvedInvert;
        resolvedColor = specified.resolvedColor;
    }
    void compute() {
        resolvedInvert = invert;
        resolvedColor = color;
    }
    bool isInvert() const {
        return resolvedInvert == Invert;
    }
    unsigned getARGB() {
        return resolvedColor;
    }
    CSSOutlineColorValueImp() :
        invert(Invert),
        color(0xff000000),
        resolvedInvert(Invert),
        resolvedColor(0xff000000)
    {}
};

class CSSOutlineShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSPaddingShorthandImp : public CSSPropertyValueImp
{
public:
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    void specify(CSSStyleDeclarationImp* self, const CSSStyleDeclarationImp* decl);
};

class CSSPositionValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Static,
        Relative,
        Absolute,
        Fixed
    };
    CSSPositionValueImp& setValue(unsigned value = Static) {
        this->value = value;
        return *this;
    }
    CSSPositionValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSPositionValueImp& positon) const {
        return value == positon.value;
    }
    bool operator!=(const CSSPositionValueImp& positon) const {
        return value != positon.value;
    }
    void specify(const CSSPositionValueImp& specified) {
        value = specified.value;
    }
    CSSPositionValueImp(unsigned initial = Static) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSQuotesValueImp : public CSSPropertyValueImp
{
    std::deque<std::pair<std::u16string, std::u16string>> quotes;
public:
    enum {
        None = 0,
    };
    void reset();
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser);
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const;
    bool operator==(const CSSQuotesValueImp& value) const {
        if (quotes.size() != value.quotes.size())
            return false;
        auto j = value.quotes.begin();
        for (auto i = quotes.begin(); i != quotes.end(); ++i) {
            if (i->first != j->first || i->second != j->second)
                return false;
        }
        return true;
    }
    bool operator!=(const CSSQuotesValueImp& value) const {
        return !(*this == value);
    }
    void specify(const CSSQuotesValueImp& specified);
    std::u16string getOpenQuote(int depth) const {
        if (depth < 0 || quotes.size() == 0)
            return u"";
        if (quotes.size() <= static_cast<size_t>(depth))
            depth = quotes.size() - 1;
        return quotes[depth].first;
    }
    std::u16string getCloseQuote(int depth) const {
        if (depth < 0 || quotes.size() == 0)
            return u"";
        if (quotes.size() <= static_cast<size_t>(depth))
            depth = quotes.size() - 1;
        return quotes[depth].second;
    }
};

class CSSTableLayoutValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Auto,
        Fixed
    };
    unsigned getValue() const {
        return value;
    }
    CSSTableLayoutValueImp& setValue(unsigned value = Auto) {
        this->value = value;
        return *this;
    }
    CSSTableLayoutValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSTableLayoutValueImp& tableLayout) const {
        return value == tableLayout.value;
    }
    bool operator!=(const CSSTableLayoutValueImp& tableLayout) const {
        return value != tableLayout.value;
    }
    void specify(const CSSTableLayoutValueImp& specified) {
        value = specified.value;
    }
    CSSTableLayoutValueImp(unsigned initial = Auto) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSTextAlignValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Left,
        Right,
        Center,
        Justify,
        Default  // Left if direction is 'ltr', Right if direction is 'rtl'
    };
    unsigned getValue() const {
        return value;
    }
    CSSTextAlignValueImp& setValue(unsigned value = Default) {
        this->value = value;
        return *this;
    }
    CSSTextAlignValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSTextAlignValueImp& textAlign) const {
        return value == textAlign.value;
    }
    bool operator!=(const CSSTextAlignValueImp& textAlign) const {
        return value != textAlign.value;
    }
    void specify(const CSSTextAlignValueImp& specified) {
        value = specified.value;
    }
    CSSTextAlignValueImp(unsigned initial = Default) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSTextDecorationValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        None = 0,
        Underline = 1,
        Overline = 2,
        LineThrough = 4,
        Blink = 8
    };
    CSSTextDecorationValueImp& setValue(unsigned value = None) {
        this->value = value;
        return *this;
    }
    virtual bool setValue(CSSStyleDeclarationImp* decl, CSSValueParser* parser) {
        unsigned decoration = None;
        std::deque<CSSParserTerm*>& stack = parser->getStack();
        for (auto i = stack.begin(); i != stack.end(); ++i)
            decoration |= (*i)->getIndex();
        setValue(decoration);
        return true;
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (value == 0)
            return Options[0];
        std::u16string cssText;
        for (int i = 0; i < 4; ++i) {
            if (value & (1u << i)) {
                if (0 < cssText.length())
                    cssText += u' ';
                cssText += Options[i + 1];
            }
        }
        return cssText;
    }
    bool operator==(const CSSTextDecorationValueImp& textDecoration) const {
        return value == textDecoration.value;
    }
    bool operator!=(const CSSTextDecorationValueImp& textDecoration) const {
        return value != textDecoration.value;
    }
    void specify(const CSSTextDecorationValueImp& specified) {
        value = specified.value;
    }
    CSSTextDecorationValueImp(unsigned initial = None) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSTextTransformValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        None,
        Capitalize,
        Uppercase,
        Lowercase
    };
    CSSTextTransformValueImp& setValue(unsigned value = None) {
        this->value = value;
        return *this;
    }
    CSSTextTransformValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSTextTransformValueImp& textTransform) const {
        return value == textTransform.value;
    }
    bool operator!=(const CSSTextTransformValueImp& textTransform) const {
        return value != textTransform.value;
    }
    void specify(const CSSTextTransformValueImp& specified) {
        value = specified.value;
    }
    CSSTextTransformValueImp(unsigned initial = None) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSUnicodeBidiValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Normal,
        Embed,
        BidiOverride
    };
    CSSUnicodeBidiValueImp& setValue(unsigned value = Normal) {
        this->value = value;
        return *this;
    }
    CSSUnicodeBidiValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSUnicodeBidiValueImp& bidi) const {
        return value == bidi.value;
    }
    bool operator!=(const CSSUnicodeBidiValueImp& bidi) const {
        return value != bidi.value;
    }
    void specify(const CSSUnicodeBidiValueImp& specified) {
        value = specified.value;
    }
    CSSUnicodeBidiValueImp(unsigned initial = Normal) :
        value(initial) {
    }
    static const char16_t* Options[];
};

class CSSVerticalAlignValueImp : public CSSPropertyValueImp
{
    CSSNumericValue value;
public:
    enum {
        Baseline,
        Sub,
        Super,
        Top,
        TextTop,
        Middle,
        Bottom,
        TextBottom
    };
    CSSVerticalAlignValueImp& setValue(float number, unsigned short unit) {
        value.setValue(number, unit);
        return *this;
    }
    CSSVerticalAlignValueImp& setValue(CSSParserTerm* term = 0) {
        if (term)
            value.setValue(term);
        else
            value.setIndex(Baseline);
        return *this;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (value.isIndex())
            return value.getSpecifiedCssText(Options);
        else
            return value.getResolvedCssText(Options);
    }
    bool operator==(const CSSVerticalAlignValueImp& align) const {
        return value == align.value;
    }
    bool operator!=(const CSSVerticalAlignValueImp& align) const {
        return value != align.value;
    }
    void specify(const CSSVerticalAlignValueImp& specified) {
        value.specify(specified.value);
    }
    void inherit(const CSSVerticalAlignValueImp& parent) {
        if (parent.value.isPercentage() || parent.value.isLength())
            value.inherit(parent.value);
        else
            value.specify(parent.value);
    }
    void compute(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    void resolve(ViewCSSImp* view, CSSStyleDeclarationImp* style);
    float getOffset(ViewCSSImp* view, CSSStyleDeclarationImp* self, LineBox* line, FontTexture* font, float point, float leading) const;
    float getOffset(ViewCSSImp* view, CSSStyleDeclarationImp* self, LineBox* line, InlineBox* text) const;
    unsigned getIndex() const {
        return value.getIndex();
    }
    unsigned getValueForCell() const {
        unsigned index = value.getIndex();
        switch (index) {
        case Baseline:
        case Top:
        case Bottom:
        case Middle:
            return index;
        default:
            return Baseline;
        }
    }
    CSSVerticalAlignValueImp() :
        value(Baseline) {
    }
    static const char16_t* Options[];
};

class CSSVisibilityValueImp : public CSSPropertyValueImp
{
    unsigned value;
    unsigned resolved;
public:
    enum {
        Visible,
        Hidden,
        Collapse
    };
    CSSVisibilityValueImp& setValue(unsigned value = Visible) {
        resolved = this->value = value;
        return *this;
    }
    CSSVisibilityValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return resolved;
    }
    bool isVisible() const {
        return resolved == Visible;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[resolved];
    }
    void specify(const CSSVisibilityValueImp& specified) {
        value = specified.value;
        resolved = specified.resolved;
    }
    void copy(const CSSVisibilityValueImp& specified) {
        resolved = specified.resolved;
    }
    void compute() {
        resolved = value;
    }
    CSSVisibilityValueImp(unsigned initial = Visible) :
        value(initial),
        resolved(initial)
    {}
    static const char16_t* Options[];
};

class CSSWhiteSpaceValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        Normal,
        Pre,
        Nowrap,
        PreWrap,
        PreLine
    };
    CSSWhiteSpaceValueImp& setValue(unsigned value = Normal) {
        this->value = value;
        return *this;
    }
    CSSWhiteSpaceValueImp& setValue(CSSParserTerm* term) {
        return setValue(term->getIndex());
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        return Options[value];
    }
    bool operator==(const CSSWhiteSpaceValueImp& ws) const {
        return value == ws.value;
    }
    bool operator!=(const CSSWhiteSpaceValueImp& ws) const {
        return value != ws.value;
    }
    void specify(const CSSWhiteSpaceValueImp& specified) {
        value = specified.value;
    }
    bool isCollapsingSpace() const {
        return isCollapsingSpace(value);
    }
    bool isBreakingLines() const {
        return isBreakingLines(value);
    }
    CSSWhiteSpaceValueImp(unsigned initial = Normal) :
        value(initial) {
    }
    static const char16_t* Options[];

    static bool isCollapsingSpace(unsigned value) {
        switch (value) {
        case Normal:
        case Nowrap:
        case PreLine:
            return true;
        default:
            return false;
        }
    }
    static bool isBreakingLines(unsigned value) {
        switch (value) {
        case Normal:
        case PreWrap:
        case PreLine:
            return true;
        default:
            return false;
        }
    }
};

class CSSZIndexValueImp : public CSSPropertyValueImp
{
    bool auto_;
    int index;
public:
    CSSZIndexValueImp& setValue(bool auto_ = true, int index = 0) {
        this->auto_ = auto_;
        if (auto_)
            this->index = 0;
        else
            this->index = index;
        return *this;
    }
    CSSZIndexValueImp& setValue(CSSParserTerm* term) {
        if (0 <= term->getIndex())
            return setValue(true);
        double value = term->getNumber();
        if (fmod(value, 1.0) != 0.0)
            return setValue(true);
        return setValue(false, static_cast<int>(value));
    }
    bool isAuto() const {
        return auto_;
    }
    int getValue() const {
        return isAuto() ? 0 : index;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (auto_)
            return u"auto";
        return CSSSerializeInteger(index);
    }
    bool operator==(const CSSZIndexValueImp& value) {
        return auto_ == value.auto_ && index == value.index;
    }
    bool operator!=(const CSSZIndexValueImp& value) {
        return auto_ != value.auto_ || index != value.index;
    }
    void specify(const CSSZIndexValueImp& specified) {
        auto_ = specified.auto_;
        index = specified.index;
    }
    CSSZIndexValueImp() :
        auto_(true),
        index(0) {
    }
};

class CSSBindingValueImp : public CSSPropertyValueImp
{
    std::u16string uri;
    unsigned value;
public:
    enum {
        None,
        Button,
        Details,  // block
        InputTextfield,
        InputPassword,
        InputDatetime,
        InputDate,
        InputMonth,
        InputWeek,
        InputTime,
        InputDatetimeLocal,
        InputNumber,
        InputRange,
        InputColor,
        InputCheckbox,
        InputRadio,
        InputFile,
        InputButton,
        Marquee,
        Meter,
        Progress,
        Select,  // multi-select list box
        Textarea,
        Keygen,
        Time,  // inline?
        Uri,
    };
    CSSBindingValueImp& setValue(unsigned value = None) {
        assert(value != Uri);
        uri.clear();
        this->value = value;
        return *this;
    }
    CSSBindingValueImp& setURL(const std::u16string uri) {
        value = Uri;
        this->uri = uri;
        return *this;
    }
    CSSBindingValueImp& setValue(CSSParserTerm* term) {
        if (0 <= term->getIndex())
            return setValue(term->getIndex());
        else
            return setURL(term->getString());
    }
    unsigned getValue() const {
        return value;
    }
    const std::u16string& getURL() const {
        return uri;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl) const {
        if (value == Uri)
            return u"url(" + CSSSerializeString(uri) + u')';
        return Options[value];
    }
    void specify(const CSSStyleDeclarationImp* decl);
    CSSBindingValueImp(unsigned initial = None) :
        value(initial) {
    }
    static const char16_t* Options[];

    bool isBlockLevel() const {
        switch (value) {
        case Details:
            return true;
        default:
            return false;
        }
    }
    bool isInlineBlock() const {
        switch (value) {
        case Button:
        case InputTextfield:
        case InputPassword:
        case InputDatetime:
        case InputDate:
        case InputMonth:
        case InputWeek:
        case InputTime:
        case InputDatetimeLocal:
        case InputNumber:
        case InputRange:
        case InputColor:
        case InputCheckbox:
        case InputRadio:
        case InputFile:
        case InputButton:
        case Marquee:
        case Meter:
        case Progress:
        case Select:  // multi-select list box
        case Textarea:
        case Keygen:
            return true;
            break;
        default:
            return false;
        }
    }
};

class HTMLAlignValueImp : public CSSPropertyValueImp
{
    unsigned value;
public:
    enum {
        None,
        Left,
        Right,
        Center,
        Justify,
    };
    HTMLAlignValueImp& setValue(unsigned value = None) {
        this->value = value;
        return *this;
    }
    unsigned getValue() const {
        return value;
    }
    virtual std::u16string getCssText(CSSStyleDeclarationImp* decl = 0) const {
        return u"";
    }
    bool operator==(const HTMLAlignValueImp& style) const {
        return value == style.value;
    }
    bool operator!=(const HTMLAlignValueImp& style) const {
        return value != style.value;
    }
    void specify(const HTMLAlignValueImp& specified) {
        value = specified.value;
    }
    HTMLAlignValueImp(unsigned initial = None) :
        value(initial)
    {
    }
};

}}}}  // org::w3c::dom::bootstrap

#endif  // CSSSPROPERTYVALUE_IMP_H
