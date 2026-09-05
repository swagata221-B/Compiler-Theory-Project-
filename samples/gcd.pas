program Gcd;
var
  a, b, t: integer;
begin
  read(a, b);
  while b <> 0 do
  begin
    { Euclidean algorithm }
    t := b;
    b := a mod b;
    a := t
  end;
  writeln('gcd = ', a)
end.
