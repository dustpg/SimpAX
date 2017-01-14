#define _CRT_SECURE_NO_WARNINGS
#include "SimpAX.h"
#include <iostream>
#include <string>
#include <cassert>
#include <chrono>
#include <thread>


static const char* const xml = u8R"xml(
<?xml version="1.0" ?>
<!--ÖÐÎÄ×¢ÊÍ£¬²âÊÔÓÃ-->
<Model>
  <Skeleton>model.skl</Skeleton>
  <Part Slot="body" Filename="body.prt"/>
  <Part Slot="shadow" Filename="shadow.prt"/>
  <Animation Slot="stand" Filename="stand.ani">
    <Event Time="1.5" Type="sound">
      <Param>breath.wav</Param>
      <Param></Param>
    </Event>
    <Event>
    </Event>
  </Animation>
  <Animation>
  </Animation>
  <Light>
    <Position>0,0,0</Position>
    <Bone>head</Bone>
    <Range>5.0</Range>
    <Color>1.0,1.0,1.0</Color>
    <AttenConst>0.0</AttenConst>
    <AttenLinear>2.0</AttenLinear>
    <AttenQuad>0.0</AttenQuad>
  </Light>
  <Light>
  </Light>
  <Window Type="Page" Name="Test">
    <Property Name="Caption" Value="test window"/>
    <Property Name="Color" Value="0,0,0"/>
    <Window Type="Button" Name="click me!"/>
    <Window Type="CheckBox" Name="check me"/>
  </Window>
</Model>
)xml";

static const char* const xml2 = u8R"xml(
<?xml version="1.0" encoding="utf-8"?>
<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">
  <Type Name="SimpAX::StrPair">
    <DisplayString>{a,[(b-a)]s8}</DisplayString>
    <Expand>
      <Item Name="[size]" ExcludeView="simple">b-a</Item>
      <ArrayItems>
        <Size>b-a</Size>
        <ValuePointer>a</ValuePointer>
      </ArrayItems>
    </Expand>
  </Type>
  <Type Name="SimpAX::CAXStream">
    <DisplayString>{{ stack-size={m_pStackTop - m_pStackBase} }}</DisplayString>
    <Expand>
      <Item Name="[size]" ExcludeView="simple">m_pStackTop - m_pStackBase</Item>
      <Item Name="[capacity]" ExcludeView="simple">m_pStackCap - m_pStackBase</Item>
      <ArrayItems>
        <Size>m_pStackTop - m_pStackBase</Size>
        <ValuePointer>m_pStackBase</ValuePointer>
      </ArrayItems>
    </Expand>
  </Type>
</AutoVisualizer>
)xml";


/// <summary>
/// xml stream 
/// </summary>
struct XmlStream : SimpAX::CAXStream {
    // string_view like
    using StrPair = SimpAX::StrPair;
private:
    // add Processing Instruction
    void add_processing(const PIs& attr) noexcept override;
    // begin element
    void begin_element(const StrPair tag) noexcept override;
    // end element
    void end_element(const StrPair tag) noexcept override;
    // add attribute
    void add_attribute(const ATTRs& attr) noexcept override;
    // add comment
    void add_comment(const StrPair) noexcept override;
    // add text
    void add_text(const StrPair) noexcept override;
};

void wait_input(int newline = false) {
    std::printf("Press Any Key to Continue  ---%c ", newline ? '\n' : ' ');
    int c; while ((c = getchar()) != '\n' && c != EOF);
}

/*
        quot    "   &#34;   &#x22;
        amp     &   &#38;   &#x26;
        apos    '   &#39;   &#x27;
        lt      <   &#60;   &#x3C;
        gt      >   &#62;   &#x3E;
*/
int main() {
    /*auto file = std::fopen("a.xml", "rb");

    assert(file);
    std::fseek(file, 0, SEEK_END);
    const auto len = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    auto str = reinterpret_cast<char*>(std::malloc(len + 1));
    assert(str);
    std::fread(str, 1, len, file); str[len] = 0;*/
    //wait_input();
    const auto* test = xml;
    /*test = R"(
<html:body>
    <html:div>
        DIV
    </html:div>
</<html:body>
)";*/
    {
        std::chrono::duration<double> diff;
        {
            XmlStream stream;
            auto start = std::chrono::high_resolution_clock::now();
            //SimpAX::CAXStream stream;
            auto code = stream.Load(test);
            auto end = std::chrono::high_resolution_clock::now();
            diff = end - start;
            if (!code.IsOk()) std::printf("\n\n__ERROR__: %d\n\n", int(code.code));
        }
        std::printf("\n\n__FINISHED__: %lfms \n\n", diff.count() * 1000.0);
    }
    //std::free(str);
    wait_input();
    return 0;
}

/// <summary>
/// Adds the processing.
/// </summary>
/// <param name="pi">The pi.</param>
/// <returns></returns>
void XmlStream::add_processing(const PIs& pi) noexcept {
    std::printf(
        "target: { %.*s } instructions: { %.*s }\n", 
        int(pi.target.b - pi.target.a), pi.target.a,
        int(pi.instructions.b - pi.instructions.a), pi.instructions.a
    );
    auto ins = pi.instructions;
    this->try_get_instruction_value("version", ins);

    ins = pi.instructions;
    this->try_get_instruction_value("href", ins);
}

/// <summary>
/// Begins the element.
/// </summary>
/// <param name="pair">The pair.</param>
/// <returns></returns>
void XmlStream::begin_element(const StrPair pair) noexcept {
    const auto stack_len = stack_end() - stack_begin();
    for (int i = 0; i != stack_len; ++i) {
        std::printf("  ");
    }
    std::printf("%.*s {\n", int(pair.b - pair.a), pair.a);
    auto name_space = pair;
    this->find_1st_namespace(name_space);
    name_space.a = name_space.b + 1;
    name_space.b = pair.b;
}

/// <summary>
/// Ends the element.
/// </summary>
/// <param name="pair">The pair.</param>
/// <returns></returns>
void XmlStream::end_element(const StrPair pair) noexcept {
    const auto stack_len = stack_end() - stack_begin();
    for (int i = 0; i != stack_len; ++i) {
        std::printf("  ");
    }
    std::printf(" }\n");
}

/// <summary>
/// Adds the attribute.
/// </summary>
/// <param name="attr">The attribute.</param>
/// <returns></returns>
void XmlStream::add_attribute(const ATTRs& attr) noexcept {
    const auto stack_len = stack_end() - stack_begin();
    for (int i = 0; i != stack_len; ++i) {
        std::printf("  ");
    }
    std::printf(
        "'%.*s' : '%.*s'\n",
        int(attr.key.b - attr.key.a), attr.key.a,
        int(attr.value.b - attr.value.a), attr.value.a
    );
}


/// <summary>
/// Adds the text.
/// </summary>
/// <param name="comment">The comment.</param>
/// <returns></returns>
void XmlStream::add_comment(const StrPair comment) noexcept {
    auto len = comment.b - comment.a;
    auto a = comment;
}

/// <summary>
/// Adds the text.
/// </summary>
/// <param name="text">The text.</param>
/// <returns></returns>
void XmlStream::add_text(const StrPair text) noexcept {
    const auto stack_len = stack_end() - stack_begin();
    for (int i = 0; i != stack_len; ++i) {
        std::printf("--");
    }
    std::printf("%.*s\n", int(text.b - text.a), text.a);
}