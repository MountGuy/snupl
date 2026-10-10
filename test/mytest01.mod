module test;
procedure predefinedTest(A : integer[][]);
var a, b, c : integer;
var d : longint;
const e : char = 'a';
const f : char[] = "string test";
begin
	a := DIM(A, 1);
	b := DOFS(A);
	c := ReadInt();
	d := ReadLong();
	WriteInt(c);
	WriteLong(d);
	WriteChar(e);
	WriteStr(f);
	WriteLn()
end predefinedTest;
procedure c();
var d : integer;
begin
	c()
end c;
end test.