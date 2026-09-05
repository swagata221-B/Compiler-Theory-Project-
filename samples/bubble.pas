program Bubble;
var
  i, j, tmp: integer;
  a: array [1..5] of integer;

procedure Swap(i, j: integer);
begin
  tmp := a[i];
  a[i] := a[j];
  a[j] := tmp
end;

begin
  for i := 1 to 5 do read(a[i]);
  for i := 1 to 4 do
    for j := 1 to 5 - i do
      if a[j] > a[j + 1] then Swap(j, j + 1);
  for i := 1 to 5 do write(a[i])
end.
