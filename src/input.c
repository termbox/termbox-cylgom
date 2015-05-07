#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "term.h"

#define BUFFER_SIZE_MAX 16

/* if s1 starts with s2 returns 1, else 0 */
static int starts_with(const char *s1, const char *s2)
{
	/* nice huh? */
	while (*s2) if (*s1++ != *s2++) {
		return 0;
	}
	return 1;
}

/* convert escape sequence to event, and return consumed bytes on success (failure == 0) */
static int parse_escape_seq(struct tb_event *event, const char *buf)
{
	// if (len >= 6 &&
	if (starts_with(buf, "\033[M")) {
		switch (buf[3] & 3) {
		case 0:
			if (buf[3] == 0x60)
				event->key = TB_KEY_MOUSE_WHEEL_UP;
			else
				event->key = TB_KEY_MOUSE_LEFT;
			break;
		case 1:
			if (buf[3] == 0x61)
				event->key = TB_KEY_MOUSE_WHEEL_DOWN;
			else
				event->key = TB_KEY_MOUSE_MIDDLE;
			break;
		case 2:
			event->key = TB_KEY_MOUSE_RIGHT;
			break;
		case 3:
			event->key = TB_KEY_MOUSE_RELEASE;
			break;
		default:
			return -6;
		}
		event->type = TB_EVENT_MOUSE; // TB_EVENT_KEY by default

		// the coord is 1,1 for upper left
		event->x = buf[4] - 1 - 32;
		event->y = buf[5] - 1 - 32;
		return 6;
	}

	/* it's pretty simple here, find 'starts_with' match and return success, else return failure */
	int i;
	for (i = 0; keys[i]; i++) {
		if (starts_with(buf, keys[i])) {
			event->ch = 0;
			event->key = 0xFFFF-i;
			return strlen(keys[i]);
		}
	}
	return 0;
}

bool extract_event(struct tb_event *event, struct ringbuffer *inbuf, int inputmode)
{
	char buf[BUFFER_SIZE_MAX+1];
	int nbytes = ringbuffer_data_size(inbuf);

	if (nbytes > BUFFER_SIZE_MAX)
		nbytes = BUFFER_SIZE_MAX;

	if (nbytes == 0)
		return false;

	ringbuffer_read(inbuf, buf, nbytes);
	buf[nbytes] = '\0';

	if (buf[0] == '\033') {
		int n = parse_escape_seq(event, buf);
		if (n != 0) {
			bool success = true;
			if (n < 0) {
				success = false;
				n = -n;
			}
			ringbuffer_pop(inbuf, 0, n);
			return success;
		} else {
			/* it's not escape sequence, then it's ALT or ESC, check inputmode */
			if (inputmode&TB_INPUT_ESC) {
				/* if we're in escape mode, fill ESC event, pop buffer, return success */
				event->ch = 0;
				event->key = TB_KEY_ESC;
				event->mod = 0;
				ringbuffer_pop(inbuf, 0, 1);
				return true;
			} else if (inputmode&TB_INPUT_ALT) {
				/* if we're in alt mode, set ALT modifier to event and redo parsing */
				event->mod = TB_MOD_ALT;
				ringbuffer_pop(inbuf, 0, 1);
				return extract_event(event, inbuf, inputmode);
			}
			assert(!"never got here");
		}
	}

	/* if we're here, this is not an escape sequence and not an alt sequence
	 * so, it's a FUNCTIONAL KEY or a UNICODE character
	 */

	/* first of all check if it's a functional key */
	if ((unsigned char)buf[0] <= TB_KEY_SPACE ||
	    (unsigned char)buf[0] == TB_KEY_BACKSPACE2)
	{
		/* fill event, pop buffer, return success */
		event->ch = 0;
		event->key = (uint16_t)buf[0];
		ringbuffer_pop(inbuf, 0, 1);
		return true;
	}

	/* feh... we got utf8 here */

	/* check if there is all bytes */
	if (nbytes >= utf8_char_length(buf[0])) {
		/* everything ok, fill event, pop buffer, return success */
		utf8_char_to_unicode(&event->ch, buf);
		event->key = 0;
		ringbuffer_pop(inbuf, 0, utf8_char_length(buf[0]));
		return true;
	}

	/* fuck!!!!1111odin1odinodin */
	return false;
}
