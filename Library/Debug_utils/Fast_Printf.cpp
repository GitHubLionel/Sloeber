#include "Fast_Printf.h"
#include <string.h>
#include <math.h>
#include <cstdarg>

// Permet d'afficher un entier de 19 chiffres (uint64_t = 18 446 744 073 509 551 615)
// Avec un uint32_t, on peut faire 9 chiffres (uint32_t = 4 294 967 295)
#define UINT_SIZE  uint32_t
//#define UINT_SIZE  uint64_t

static char DECIMAL_SEPARATOR = ',';

/**
 * Défini le séparateur décimal à utiliser. Par défaut, c'est la virgule.
 */
void Fast_Set_Decimal_Separator(char decimal)
{
	DECIMAL_SEPARATOR = decimal;
}

/**
 * Fonction permettant de se positionner au début ou à la fin d'une chaine
 * Renvoie un pointeur sur le premier caractère libre et la longueur de la chaine
 * pos_def indique le positionnement dans la chaine
 */
char* Fast_Pos_Buffer(char *buffer, Buffer_Pos_Def pos_def, uint16_t *buffer_len)
{
	*buffer_len = 0;
	if (pos_def == Buffer_End)
	{
		while (*buffer++ != 0)
		{
			(*buffer_len)++;
		}
		buffer--;
	}
	return buffer;
}

/**
 * Idem fonction précédente mais permet de concaténer une chaine
 */
char* Fast_Pos_Buffer(char *buffer, const char *string, Buffer_Pos_Def pos_def, uint16_t *buffer_len)
{
	char ch;
	*buffer_len = 0;

	if (pos_def == Buffer_End)
	{
		while (*buffer++ != 0)
		{
			(*buffer_len)++;
		}
		buffer--;
	}

	if (strlen(string) != 0)
	{
		while ((ch = *string++) != 0)
		{
			*buffer++ = ch;
			(*buffer_len)++;
		}
		*buffer = 0; // fini la chaine
	}

	return buffer;
}

/**
 * Fonction de conversion d'un float en string
 * buffer : buffer pour le résultat
 * value : la valeur à convertir
 * prec : la précision souhaitée. ATTENTION, on est limité à 9 caractères pour la partie entière + décimale (voir define UINT_SIZE)
 * firststring, laststring : chaine de caractères à rajouter avant et après le nombre
 * pos_def : indique si le résultat de la fonction doit pointer sur le début (premier caractère) ou la fin de la chaine (le 0).
 *           Permet de faire rapidement des concaténations
 * end_len : retourne la longueur de la chaine finale (doit être initialisé à zéro, sinon cumule le résultat)
 * ATTENTION : aucune vérification d'espace mémoire n'est faite
 * DECIMAL_SEPARATOR définie le séparateur décimal
 * La chaine est terminée par 0.
 * Exemple :
 * pbuffer = Fast_Printf(dest, value, prec, "", "", Buffer_End, &len);
 */
char* Fast_Printf(char *buffer, float value, uint8_t prec, const char *firststring,
		const char *laststring, Buffer_Pos_Def pos_def, uint16_t *end_len)
{
	static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000}; // Les puissances de 10
//  static const char *chiffre = "0123456789";  // Les chiffres
	float f_tmp;
	double d_tmp;  // Double pour la précision
	uint8_t entier = 0, len = 0, pos = 0;
	UINT_SIZE number;
	char *pbuffer_origine = buffer;  // Le début du buffer initial
	char *pbuffer;
	char ch;

	// Première chaine
	if (strlen(firststring) != 0)
	{
		while ((ch = *firststring++) != 0)
		{
			*buffer++ = ch;
		}
	}

	// Le test NaN
	// Pour simplifier, on met 0
	if (isnan(value))
	{
		value = 0.0;
	}

	// Test nombre négatif
	if (value < 0.0)
	{
		value = 0.0 - value;
		// On met le '-'
		*buffer++ = '-';
	}

	// Détermine la longueur de la partie entière
	f_tmp = value;
	if (f_tmp < 1.0)
		entier = 1;  // on veut écrire 0.xx et pas .xx
	else
		while (f_tmp >= 1.0)  // entier déjà initialisé
		{
			entier++;
			f_tmp /= 10.0;
		}

	// On transforme en un entier
	d_tmp = value * pow10[prec];
	number = (UINT_SIZE) (d_tmp);
	if ((d_tmp - number) >= 0.5)
		number++;  // On arrondi la dernière décimale

	// Longueur de la chaine finale : partie entière + '.' + precision partie décimale
	// Le '-' si nécessaire est déjà en place
	// Si la précision est de 1 on évite d'avoir des xx.0
	if ((prec == 1) && (number % 10 == 0))
	{
		prec = 0;
		number /= 10;
	}

	len = entier + prec + (uint8_t) (prec != 0);

	// On se positionne en bout de la chaine
	pbuffer = buffer + len - 1;

	while (pos < len)  // pos déjà initialisé
	{
		if ((pos == prec) && (prec != 0))  // le point décimal
		{
			*pbuffer = DECIMAL_SEPARATOR;
		}
		else
		{
			if (number == 0)
				*pbuffer = '0';
			else
			{
				*pbuffer = (char) (48 + (number % 10)); // ou chiffre[number % 10];
				number /= 10;
			}
		}
		pos++;
		pbuffer--;
	}

	// Dernière chaine
	pbuffer = buffer + len;
	if (strlen(laststring) != 0)
	{
		while ((ch = *laststring++) != 0)
		{
			*pbuffer++ = ch;
		}
	}
	*pbuffer = 0;  // fini la chaine

	// Longueur de la chaine final
	*end_len += (pbuffer - pbuffer_origine);

	if (pos_def == Buffer_End)
		return pbuffer;
	else
		return pbuffer_origine;
}

/**
 * Concaténe une série de float (version varadique)
 * buffer : buffer pour le résultat.
 * prec : la précision souhaitée.
 * separator : le séparateur entre chaque float
 * pos_def : position du pointeur dans la chaine finale
 * keep_last_separator : conserve le separateur en fin de chaîne
 * count : nombre de float
 */
char* Fast_Printf(char *buffer, uint8_t prec, const char *separator, Buffer_Pos_Def pos_def,
		bool keep_last_separator, uint8_t count, ...)
{
	char *pbuffer_origine = buffer;  // Le début du buffer initial
	char *pbuffer = buffer;
	va_list args;
	va_start(args, count);
	uint16_t len;

	for (uint8_t i = 0; i < count; i++)
	{
		double val = va_arg(args, double);
		pbuffer = Fast_Printf(pbuffer, val, prec, "", separator, Buffer_End, &len);
	}

	// Supprimer le dernier séparateur
	if (!keep_last_separator)
		for (uint8_t i = 0; i < strlen(separator); i++)
			pbuffer--;
	*pbuffer = 0;

	va_end(args);

	if (pos_def == Buffer_End)
		return pbuffer;
	else
		return pbuffer_origine;
}

/**
 * Concaténe une série de float (version list)
 * buffer : buffer pour le résultat.
 * prec : la précision souhaitée.
 * separator : le séparateur entre chaque float
 * pos_def : position du pointeur dans la chaine finale
 * keep_last_separator : conserve le separateur en fin de chaîne
 * values : list de float, format {v1, v2, ... }
 */
char* Fast_Printf(char *buffer, uint8_t prec, const char *separator, Buffer_Pos_Def pos_def,
		bool keep_last_separator, std::initializer_list<double> values)
{
	char *pbuffer_origine = buffer;  // Le début du buffer initial
	char *pbuffer = buffer;
	uint16_t len;

	for (auto val : values)
	{
		pbuffer = Fast_Printf(pbuffer, val, prec, "", separator, Buffer_End, &len);
	}

	// Supprimer le dernier séparateur
	if (!keep_last_separator)
		for (uint8_t i = 0; i < strlen(separator); i++)
			pbuffer--;
	*pbuffer = 0;

	if (pos_def == Buffer_End)
		return pbuffer;
	else
		return pbuffer_origine;
}

/**
 * Add \r\n to the end of buffer
 * pos_def : the actual position of the pointer in the buffer
 */
void Fast_Add_EndLine(char *buffer, Buffer_Pos_Def pos_def)
{
	char *pbuffer = buffer;
	if (pos_def == Buffer_Begin)
	{
		pbuffer += strlen(buffer);
	}

	*pbuffer++ = '\r';
	*pbuffer++ = '\n';
	*pbuffer = 0;
}

// ********************************************************************************
// End of file
// ********************************************************************************

