CREATE OR REPLACE FUNCTION retourner_un()
RETURNS integer AS $$
DECLARE
    resultat integer := 1;
BEGIN
    RETURN resultat;
END;
$$ LANGUAGE plpgsql;

