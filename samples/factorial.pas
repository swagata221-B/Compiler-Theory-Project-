program Factorial;
var
  n, result: integer;

function Fact(x: integer): integer;
var
  i, acc: integer;
begin
  acc := 1;
  i := 1;
  while i <= x do
  begin
    acc := acc * i;
    i := i + 1
  end;
  Fact := acc
end;

begin
  read(n);
  result := Fact(n);
  writeln(result)
end.
