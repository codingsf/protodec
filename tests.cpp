#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <gtest/gtest.h>
#include "protoraw.hpp"

///*

TEST(RawMessage, asciiString) {
    std::string s;
    s = "1234566789adfsdfsdfsdfSZZZZ ds ?? 1";
    ASSERT_TRUE(RawMessage::itsAsciiString(s.c_str(), s.c_str() + s.length())); 
    s = "12345667\u000589adf\u0001sdfsdfsdfSZZZZ ds ?? 1";
    ASSERT_FALSE(RawMessage::itsAsciiString(s.c_str(), s.c_str() + s.length())); 
}

TEST(RawMessage, varIntRead) {
    int64_t value = 0;
    unsigned char data[] = { 0xbd, 0x01 };
    const unsigned char *ptr = RawMessage::readVarint(data, data + 2, value);
    ASSERT_EQ(value, 189);
    ASSERT_EQ(ptr, data+2);
}

TEST(RawMessage, varIntWrite) {
    unsigned char data[2], *ptr = NULL;

    // single zero value
    data[0] = 0xff; data[1] = 0xff;
    ptr = RawMessage::writeVarint(0, data, data + 2);
    ASSERT_EQ(data[0], 0x00);
    ASSERT_EQ(data[1], 0xff);
    ASSERT_EQ(ptr, data+1);

    // 189 -> bd 01
    data[0] = 0x00; data[1] = 0x00;
    ptr = RawMessage::writeVarint(189, data, data + 2);
    ASSERT_EQ(data[0], 0xbd);
    ASSERT_EQ(data[1], 0x01);
    ASSERT_EQ(ptr, data+2);

    // not enough bytes to store
    data[0] = 0x00; data[1] = 0x00;
    ptr = RawMessage::writeVarint(189, data, data + 1);
    ASSERT_EQ(data[0], 0xbd);
    ASSERT_EQ(data[1], 0x00);
    ASSERT_EQ(ptr, data+1);
}

TEST(RawMessage, parsing) {
    {
    unsigned char data[] = {
        0x0a, 0x04, '0', '1', '2', '3'
    };
    size_t len = (sizeof(data)/sizeof(*data));
    RawMessage msg;
    msg.parse(data, data + len);
    ASSERT_EQ(msg.items().size(), 1);
    ASSERT_EQ(msg.items().count(1), 1);
    }
    {
    unsigned char data[] = {
        0x0a, 0x05, '0', '1', '2', '3', '4',
        0x0a, 0x04, 'a', 'b', 'c', 'd',
        0x0a, 0x03, 'X', 'Y', 'Z'
    };
    size_t len = (sizeof(data)/sizeof(*data));
    RawMessage msg;
    msg.parse(data, data + len);
    ASSERT_EQ(msg.items().size(), 1);
    ASSERT_EQ(msg[1]->asStringMap(1), "01234");
    ASSERT_EQ(msg[1]->asStringMap(2), "abcd");
    ASSERT_EQ(msg[1]->asStringMap(3), "XYZ");
    }
}

TEST(RawMessage, sizeInBytes) {
    {
    unsigned char data[] = {
        0x0a, 0x04, '0', '1', '2', '3'
    };
    size_t len  = (sizeof(data)/sizeof(*data));
    RawMessage msg;
    msg.parse(data, data + len);
    ASSERT_EQ(msg.getSizeInBytes(msg.items()), len);
    }
    {
    unsigned char data[] = {
        0x0a, 0x05, '0', '1', '2', '3', '4',
        0x0a, 0x04, 'a', 'b', 'c', 'd',
        0x0a, 0x03, 'X', 'Y', 'Z'
    };
    size_t len = (sizeof(data)/sizeof(*data));
    RawMessage msg;
    msg.parse(data, data + len);
    ASSERT_EQ(msg.getSizeInBytes(msg.items()), len);
    }
}

TEST(RawMessage, printing) {
    {
    unsigned char data[] = {
        0x0a, 0x05, '0', '1', '2', '3', '4',
        0x0a, 0x04, 'a', 'b', 'c', 'd',
        0x0a, 0x03, 'X', 'Y', 'Z'
    };
    std::string expected = "1 [\n"
                           "\t1: \"01234\"\n"
                           "\t2: \"abcd\"\n"
                           "\t3: \"XYZ\"\n"
                           "]\n";
    size_t len = (sizeof(data)/sizeof(*data));
    std::stringstream ss;
    RawMessage msg;
    msg.parse(data, data + len);
    msg.print(ss);
    ASSERT_EQ(ss.str(), expected);
    }
}

//*/

TEST(Schema, print) {
    {
    unsigned char data[] = {
        0x0a, 0x05, '0', '1', '2', '3', '4',
        0x0a, 0x04, 'a', 'b', 'c', 'd',
        0x0a, 0x03, 'X', 'Y', 'Z'
    };
    std::string expected = "package ProtodecMessages;\n"
                           "\n"
                           "message MSG1 {\n"
                           "\trepeated string fld1 = 1;\n"
                           "}\n";
    size_t len = (sizeof(data)/sizeof(*data));
    RawMessage msg;
    std::stringstream ss;
    ASSERT_TRUE(msg.parse(data, data + len));
    Schema::print(msg, ss);
    ASSERT_EQ(ss.str(), expected);
    }{
    unsigned char data[] = {
        0x0a, 0x05, '0', '1', '2', '3', '4'
    };
    std::string expected = "package ProtodecMessages;\n"
                           "\n"
                           "message MSG1 {\n"
                           "\trequired string fld1 = 1;\n"
                           "}\n";
    size_t len = (sizeof(data)/sizeof(*data));
    RawMessage msg;
    std::stringstream ss;
    ASSERT_TRUE(msg.parse(data, data + len));
    Schema::print(msg, ss);
    ASSERT_EQ(ss.str(), expected);
    }
}

TEST(Serialized_pb, find) {
    char data[] = "BEGINOFGARBIGEGARBIGEGARBIGEGARBIGEGARBIGEGARBIGE"
                  "GARBIGEGARBIGEGARBIGEGARBIGEGARBIGEGARBIGEGARBIGE"
                  "\n\x11\x61\x64\x64ressbook.proto\x12\x08tutorial\"\xda\x01\n\x06Person\x12\x0c\n\x04name\x18\x01 \x02(\t\x12\n\n\x02id\x18\x02 \x02(\x05\x12\r\n\x05\x65mail\x18\x03 \x01(\t\x12+\n\x05phone\x18\x04 \x03(\x0b\x32\x1c.tutorial.Person.PhoneNumber\x1aM\n\x0bPhoneNumber\x12\x0e\n\x06number\x18\x01 \x02(\t\x12.\n\x04type\x18\x02 \x01(\x0e\x32\x1a.tutorial.Person.PhoneType:\x04HOME\"+\n\tPhoneType\x12\n\n\x06MOBILE\x10\x00\x12\x08\n\x04HOME\x10\x01\x12\x08\n\x04WORK\x10\x02\"/\n\x0b\x41\x64\x64ressBook\x12 \n\x06person\x18\x01 \x03(\x0b\x32\x10.tutorial.Person"
                  "\0ENDOFGARBIGEGARBIGEGARBIGEGARBIGEGARBIGEGARBIGE"
                  "GARBIGEGARBIGEGARBIGEGARBIGEGARBIGEGARBIGEGARBIGE";
    const unsigned char *pB = (unsigned char*) data, *pE = pB + (sizeof(data)/sizeof(*data)), *ptr;
    ptr = Serialized_pb::findSerializedPB(pB, pE);
    ASSERT_TRUE(ptr != NULL);
    ASSERT_EQ(strncmp((char*)pE+1, "ENDOFGARBIGE", 12), 0);
    ASSERT_TRUE(RawMessage::isValidMessage(ptr, pE));
}

TEST(Serialized_pb, grab) {
    RawMessage msg;
    char data[] = "\n\x11\x61\x64\x64ressbook.proto\x12\x08tutorial\"\xda\x01\n\x06Person\x12\x0c\n\x04name\x18\x01 \x02(\t\x12\n\n\x02id\x18\x02 \x02(\x05\x12\r\n\x05\x65mail\x18\x03 \x01(\t\x12+\n\x05phone\x18\x04 \x03(\x0b\x32\x1c.tutorial.Person.PhoneNumber\x1aM\n\x0bPhoneNumber\x12\x0e\n\x06number\x18\x01 \x02(\t\x12.\n\x04type\x18\x02 \x01(\x0e\x32\x1a.tutorial.Person.PhoneType:\x04HOME\"+\n\tPhoneType\x12\n\n\x06MOBILE\x10\x00\x12\x08\n\x04HOME\x10\x01\x12\x08\n\x04WORK\x10\x02\"/\n\x0b\x41\x64\x64ressBook\x12 \n\x06person\x18\x01 \x03(\x0b\x32\x10.tutorial.Person";
    size_t len = (sizeof(data)/sizeof(*data));
    ASSERT_TRUE(msg.parse((unsigned char*) data, (unsigned char*) data + len));

    std::stringstream ss;
    std::string expected = "package tutorial;\n"
                           "message Person {\n"
                           "\tenum PhoneType {\n"
                           "\t\tMOBILE = 0;\n"
                           "\t\tHOME = 1;\n"
                           "\t\tWORK = 2;\n"
                           "\t}\n"
                           "\tmessage PhoneNumber {\n"
                           "\t\trequired string number = 1;\n"
                           "\t\toptional .tutorial.Person.PhoneType type = 2 [default = HOME];\n"
                           "\t}\n"
                           "\trequired string name = 1;\n"
                           "\trequired int32 id = 2;\n"
                           "\toptional string email = 3;\n"
                           "\trepeated .tutorial.Person.PhoneNumber phone = 4;\n"
                           "}\n"
                           "message AddressBook {\n"
                           "\trepeated .tutorial.Person person = 1;\n"
                           "}\n";
    Serialized_pb::printMessagesFromSerialized(msg, ss);
    ASSERT_EQ(ss.str(), expected);
}

int main(int argc, char ** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}