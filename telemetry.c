/* **************************************************************** *
 *                                                                  *
 *  APRX -- 2nd generation receive-only APRS-i-gate with            *
 *          minimal requirement of esoteric facilities or           *
 *          libraries of any kind beyond UNIX system libc.          *
 *                                                                  *
 * (c) Matti Aarnio - OH2MQK,  2007,2008                            *
 *                                                                  *
 * **************************************************************** */

#include "aprx.h"

int telemetry_interval = 20 * 60;	/* 20 minutes */
time_t telemetry_time;
int telemetry_seq;

int telemetry_prepoll(struct aprxpolls *app)
{
	if (telemetry_time == 0)
		telemetry_time = now + 30;

	if (app->next_timeout > telemetry_time)
		app->next_timeout = telemetry_time;

	return 0;
}

int telemetry_postpoll(struct aprxpolls *app)
{
	int i, j, k, t;
	char buf[200], *s;
	char beaconaddr[30];
	long erlmax;
	float erlcapa;

	if (telemetry_time > now)
		return 0;	/* Not yet ... */

	telemetry_time += telemetry_interval;

	for (i = 0; i < ErlangLinesCount; ++i) {
		struct erlangline *E = ErlangLines[i];

		sprintf(beaconaddr, "%s>RXTLM-%d", E->name, i + 1);
		s = buf;
		s += sprintf(s, "T#%03d,", telemetry_seq & 255);

		erlmax = 0;
		k = E->e1_cursor;
		t = E->e1_max;
		if (t > 20)
			t = 20;	/* Up to 20 of 1 minute samples */
		erlcapa = E->erlang_capa;
		for (j = 0; j < t; ++j) {
			--k;
			if (k < 0)
				k = E->e1_max - 1;
			if (E->e1[k].bytes_rx > erlmax)
				erlmax = E->e1[k].bytes_rx;
		}
		k = (int) (200.0 / erlcapa * (float) erlmax);
		if (k > 255)
			k = 255;
		s += sprintf(s, "%03d,", k);

		erlmax = 0;
		k = E->e10_cursor;
		t = E->e10_max;
		if (t > 2)
			t = 2;	/* Up to 2 of 10 minute samples */
		erlcapa = E->erlang_capa;
		for (j = 0; j < t; ++j) {
			--k;
			if (k < 0)
				k = E->e10_max - 1;
			if (E->e10[k].bytes_rx > erlmax)
				erlmax = E->e10[k].bytes_rx;
		}
		k = (int) (20.0 / erlcapa * (float) erlmax);
		if (k > 255)
			k = 255;
		s += sprintf(s, "%03d,", k);

		erlmax = 0;
		k = E->e1_cursor;
		t = E->e1_max;
		if (t > 20)
			t = 20;	/* Up to 2 of 10 minute samples */
		for (j = 0; j < t; ++j) {
			--k;
			if (k < 0)
				k = E->e1_max - 1;
			erlmax += E->e1[k].packets_rx;
		}
		s += sprintf(s, "%03d,", (int) erlmax);

		erlmax = 0;
		k = E->e1_cursor;
		t = E->e1_max;
		if (t > 20)
			t = 20;	/* Up to 2 of 10 minute samples */
		for (j = 0; j < t; ++j) {
			--k;
			if (k < 0)
				k = E->e1_max - 1;
			erlmax += E->e1[k].packets_rxdrop;
		}
		s += sprintf(s, "%03d,", (int) erlmax);

		/* Tail filler */
		s += sprintf(s, "000,00000000");

		/* _NO_ ending CRLF, the APRSIS subsystem adds it. */

		/* Send those (net)beacons.. */
		aprsis_queue(beaconaddr, mycall, buf, (int) (s - buf));

		if ((telemetry_seq % 128) == 0) {

			/* Send at start, and every about 2 days.. */

			s = buf + sprintf(buf,
					  ":%-9s:PARM.Max1m,Max10m,RxPkts,DropRxPkts",
					  E->name);
			aprsis_queue(beaconaddr, mycall, buf,
				     (int) (s - buf));

			s = buf + sprintf(buf,
					  ":%-9s:UNIT.Erlang,Erlang,count,count",
					  E->name);
			aprsis_queue(beaconaddr, mycall, buf,
				     (int) (s - buf));

			s = buf + sprintf(buf,
					  ":%-9s:EQNS.0,0.005,0,0,0.005,0,0,1,0,0,1,0",
					  E->name);
			aprsis_queue(beaconaddr, mycall, buf,
				     (int) (s - buf));
		}
	}
	++telemetry_seq;

	return 0;
}