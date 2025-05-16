  create table if not exists t ( i int);
  truncate t;
  insert into t select i from generate_series(1,10) i;

  CREATE OR REPLACE FUNCTION public.fn()
  RETURNS TABLE(i integer)
  LANGUAGE sql AS
  $function$
  select * from t;
  $function$
