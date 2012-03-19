#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <xbyak/xbyak.h>

Xbyak::uint8 bufL[4096 * 32];
Xbyak::uint8 bufS[4096 * 2];

struct MyAllocator : Xbyak::Allocator {
	Xbyak::uint8 *alloc(size_t size)
	{
		if (size < sizeof(bufS)) {
			printf("use bufS(%d)\n", (int)size);
			return bufS;
		}
		if (size < sizeof(bufL)) {
			printf("use bufL(%d)\n", (int)size);
			return bufL;
		}
		fprintf(stderr, "no memory %d\n", (int)size);
		exit(1);
	}
	void free(Xbyak::uint8 *)
	{
	}
} myAlloc;

void dump(const std::string& m)
{
	printf("size=%d\n     ", (int)m.size());
	for (int i = 0; i < 16; i++) {
		printf("%02x ", i);
	}
	printf("\n     ");
	for (int i = 0; i < 16; i++) {
		printf("---");
	}
	printf("\n");
	for (size_t i = 0; i < m.size(); i++) {
		if ((i % 16) == 0) printf("%04x ", (int)(i / 16));
		printf("%02x ", (unsigned char)m[i]);
		if ((i % 16) == 15) putchar('\n');
	}
	putchar('\n');
}

void diff(const std::string& a, const std::string& b)
{
	puts("diff");
	if (a.size() != b.size()) printf("size diff %d %d\n", (int)a.size(), (int)b.size());
	for (size_t i = 0; i < a.size(); i++) {
		if (a[i] != b[i]) {
			printf("diff %d(%04x) %02x %02x\n", (int)i, (int)i, (unsigned char)a[i], (unsigned char)b[i]);
		}
	}
	puts("end");
}

struct Test2 : Xbyak::CodeGenerator {
	explicit Test2(int size, int count, void *mode)
		: CodeGenerator(size, mode, &myAlloc)
	{
		using namespace Xbyak;
		inLocalLabel();
		mov(ecx, count);
		xor(eax, eax);
	L(".lp");
		for (int i = 0; i < count; i++) {
			L(Label::toStr(i).c_str());
			add(eax, 1);
			int to = 0;
			if (i < count / 2) {
				to = count - 1 - i;
			} else {
				to = count  - i;
			}
			if (i == count / 2) {
				jmp(".exit", T_NEAR);
			} else {
				jmp(Label::toStr(to).c_str(), T_NEAR);
			}
		}
	L(".exit");
		sub(ecx, 1);
		jnz(".lp", T_NEAR);
		ret();
		outLocalLabel();
	}
};

struct Test1 : Xbyak::CodeGenerator {
	explicit Test1(int size, void *mode)
		: CodeGenerator(size, mode)
	{
		using namespace Xbyak;
		inLocalLabel();
		outLocalLabel();
		jmp(".x");
		for (int i = 0; i < 10; i++) {
			nop();
		}
	L(".x");
		ret();
	}
};
void test1()
{
	std::string fm, gm;
	Test1 fc(1024, 0);
	Test1 gc(5, Xbyak::AutoGrow);
	gc.ready();
	fm.assign((const char*)fc.getCode(), fc.getSize());
	gm.assign((const char*)gc.getCode(), gc.getSize());
//	dump(fm);
//	dump(gm);
	diff(gm, gm);
}

void test2()
{
	std::string fMem, gMem;
	const int count = 50;
	int ret;
	Test2 fCode(1024 * 64, count, 0);
	ret = ((int (*)())fCode.getCode())();
	if (ret != count * count) {
		printf("err ret=%d, %d\n", ret, count * count);
	} else {
		puts("ok");
	}
	fMem.assign((const char*)fCode.getCode(), fCode.getSize());
	Test2 gCode(10, count, Xbyak::AutoGrow);
	gCode.ready();
#if 0
	ret = ((int (*)())gCode.getCode())();
	if (ret != count * count) {
		printf("err ret=%d, %d\n", ret, count * count);
	} else {
		puts("ok");
	}
#endif
	gMem.assign((const char*)gCode.getCode(), gCode.getSize());
//	dump(fMem);
//	dump(gMem);
	diff(fMem, gMem);
}

int main()
{
	try {
		test1();
		test2();
	} catch (Xbyak::Error err) {
		printf("ERR:%s(%d)\n", Xbyak::ConvertErrorToString(err), err);
	} catch (...) {
		printf("unknown error\n");
	}
}

