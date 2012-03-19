#include <stdio.h>
#include <string.h>
#include "xbyak/xbyak.h"
#define NUM_OF_ARRAY(x) (sizeof(x) / sizeof(x[0]))

using namespace Xbyak;

struct TestJmp : public CodeGenerator {
	void putNop(int n)
	{
		for (int i = 0; i < n; i++) {
			nop();
		}
	}
/*
     4                                  X0:
     5 00000004 EBFE                    jmp short X0
     6
     7                                  X1:
     8 00000006 <res 00000001>          dummyX1 resb 1
     9 00000007 EBFD                    jmp short X1
    10
    11                                  X126:
    12 00000009 <res 0000007E>          dummyX126 resb 126
    13 00000087 EB80                    jmp short X126
    14
    15                                  X127:
    16 00000089 <res 0000007F>          dummyX127 resb 127
    17 00000108 E97CFFFFFF              jmp near X127
    18
    19 0000010D EB00                    jmp short Y0
    20                                  Y0:
    21
    22 0000010F EB01                    jmp short Y1
    23 00000111 <res 00000001>          dummyY1 resb 1
    24                                  Y1:
    25
    26 00000112 EB7F                    jmp short Y127
    27 00000114 <res 0000007F>          dummyY127 resb 127
    28                                  Y127:
    29
    30 00000193 E980000000              jmp near Y128
    31 00000198 <res 00000080>          dummyY128 resb 128
    32                                  Y128:
*/
	TestJmp(int offset, bool isBack, bool isShort)
	{
		char buf[32];
		static int count = 0;
		if (isBack) {
			sprintf(buf, "L(\"X%d\");\n", count);
			L(buf);
			putNop(offset);
			jmp(buf);
		} else {
			sprintf(buf, "L(\"Y%d\");\n", count);
			if (isShort) {
				jmp(buf);
			} else {
				jmp(buf, T_NEAR);
			}
			putNop(offset);
			L(buf);
		}
		count++;
	}
};

void test1()
{
	static const struct Tbl {
		int offset;
		bool isBack;
		bool isShort;
		const char *result;
	} tbl[] = {
		{ 0, true, true, "EBFE" },
		{ 1, true, true, "EBFD" },
		{ 126, true, true, "EB80" },
		{ 127, true, false, "E97CFFFFFF" },
		{ 0, false, true, "EB00" },
		{ 1, false, true, "EB01" },
		{ 127, false, true, "EB7F" },
		{ 128, false, false, "E980000000" },
	};
	for (size_t i = 0; i < NUM_OF_ARRAY(tbl); i++) {
		const Tbl *p = &tbl[i];
		TestJmp jmp(p->offset, p->isBack, p->isShort);
		const uint8 *q = (const uint8*)jmp.getCode();
		char buf[32];
		if (p->isBack) q += p->offset; /* skip nop */
		for (size_t j = 0; j < jmp.getSize() - p->offset; j++) {
			sprintf(&buf[j * 2], "%02X", q[j]);
		}
		if (strcmp(buf, p->result) != 0) {
			printf("error %d assume:%s, err=%s\n", (int)i, p->result, buf);
		} else {
			printf("ok %d\n", (int)i);
		}
	}
}

struct TestJmp2 : public CodeGenerator {
	void putNop(int n)
	{
		for (int i = 0; i < n; i++) {
			nop();
		}
	}
/*
  1 00000000 90                      nop
  2 00000001 90                      nop
  3                                  f1:
  4 00000002 <res 0000007E>          dummyX1 resb 126
  6 00000080 EB80                     jmp f1
  7
  8                                  f2:
  9 00000082 <res 0000007F>          dummyX2 resb 127
 11 00000101 E97CFFFFFF               jmp f2
 12
 13
 14 00000106 EB7F                    jmp f3
 15 00000108 <res 0000007F>          dummyX3 resb 127
 17                                  f3:
 18
 19 00000187 E980000000              jmp f4
 20 0000018C <res 00000080>          dummyX4 resb 128
 22                                  f4:
*/
	explicit TestJmp2(void *p)
		: Xbyak::CodeGenerator(8192, p)
	{
		inLocalLabel();
		nop();
		nop();
	L(".f1");
		putNop(126);
		jmp(".f1");
	L(".f2");
		putNop(127);
		jmp(".f2", T_NEAR);

		jmp(".f3");
		putNop(127);
	L(".f3");
		jmp(".f4", T_NEAR);
		putNop(128);
	L(".f4");
		outLocalLabel();
	}
};

void test2()
{
	puts("test2");
	std::string ok;
	ok.resize(0x18C + 128, 0x90);
	ok[0x080] = 0xeb;
	ok[0x081] = 0x80;

	ok[0x101] = 0xe9;
	ok[0x102] = 0x7c;
	ok[0x103] = 0xff;
	ok[0x104] = 0xff;
	ok[0x105] = 0xff;

	ok[0x106] = 0xeb;
	ok[0x107] = 0x7f;

	ok[0x187] = 0xe9;
	ok[0x188] = 0x80;
	ok[0x189] = 0x00;
	ok[0x18a] = 0x00;
	ok[0x18b] = 0x00;
	for (int j = 0; j < 2; j++) {
		TestJmp2 c(j == 0 ? 0 : Xbyak::AutoGrow);
		c.ready();
		std::string m((const char*)c.getCode(), c.getSize());
		if (m.size() != ok.size()) {
			printf("test2 err %d %d\n", (int)m.size(), (int)ok.size());
		} else {
			if (m != ok) {
				for (size_t i = 0; i < m.size(); i++) {
					if (m[i] != ok[i]) {
						printf("diff 0x%03x %02x %02x\n", (int)i, (unsigned char)m[i], (unsigned char)ok[i]);
					}
				}
			} else {
				puts("ok");
			}
		}
	}
}

#if !defined(_WIN64) && !defined(__x86_64__)
	#define ONLY_32BIT
#endif

#ifdef ONLY_32BIT
int add5(int x) { return x + 5; }
int add2(int x) { return x + 2; }

struct Grow : Xbyak::CodeGenerator {
	Grow(int dummySize)
		: Xbyak::CodeGenerator(128, Xbyak::AutoGrow)
	{
		mov(eax, 100);
		push(eax);
		call((void*)add5);
		add(esp, 4);
		push(eax);
		call((void*)add2);
		add(esp, 4);
		ret();
		for (int i = 0; i < dummySize; i++) {
			db(0);
		}
	}
};

void test3()
{
	for (int dummySize = 0; dummySize < 40000; dummySize += 10000) {
		printf("dummySize=%d\n", dummySize);
		Grow g(dummySize);
		g.ready();
		int (*f)() = (int (*)())g.getCode();
		int x = f();
		const int ok = 107;
		if (x != ok) {
			printf("err %d assume %d\n", x, ok);
		} else {
			printf("ok\n");
		}
	}
}
#endif

int main()
{
	try {
		test1();
		test2();
#ifdef ONLY_32BIT
		test3();
#endif
	} catch (Xbyak::Error err) {
		printf("ERR:%s(%d)\n", Xbyak::ConvertErrorToString(err), err);
	} catch (...) {
		printf("unknown error\n");
	}
}
