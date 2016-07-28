/*
 * Copyright (c) 2016, Natacha Porté
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

const settings = {  /* "name in local storage": "form input parameter" */
   "event-list":    "ev",
   "begin-prefix":  "bpre",
   "end-prefix":    "epre",
};

var cfg_endpoint = null;
var cfg_data_field = null;
var cfg_extra_fields = [];

function encodeStored(names) {
   var result = "?v=dev";
   for (var key in names) {
      var value = localStorage.getItem(key);
      if (value != null) {
         result = result + "&" + names[key] + "=" + encodeURIComponent(value);
      }
   }

   if (cfg_endpoint) {
      result += "&url=" + encodeURIComponent(cfg_endpoint);
   }
   if (cfg_data_field) {
      result += "&data_field=" + encodeURIComponent(cfg_data_field);
   }
   if (cfg_extra_fields.length > 0) {
      result += "&extra=" + cfg_extra_fields.join(",");
   }

   console.log("Encoded from storage: " + result);
   return result;
}

Pebble.addEventListener("ready", function() {
   var str_extra_fields = localStorage.getItem("extra-fields");
   cfg_extra_fields = str_extra_fields ? str_extra_fields.split(",") : [];

   cfg_endpoint = localStorage.getItem("url");
   cfg_data_field = localStorage.getItem("data-field");

   console.log("Life-Log PebbleKit JS ready!");
});

Pebble.addEventListener("showConfiguration", function() {
   Pebble.openURL("https://cdn.rawgit.com/faelys/life-log/v1.0/config.html" + encodeStored(settings));
});

Pebble.addEventListener("webviewclosed", function(e) {
   var configData = JSON.parse(e.response);

   for (var key in settings) {
      localStorage.setItem(key, decodeURIComponent(configData[key]));
   }

   if (configData["extra-fields"] !== null) {
      cfg_extra_fields = configData["extra-fields"]
       ? configData["extra-fields"].split(",") : [];
      localStorage.setItem("extra-fields", cfg_extra_fields.join(","));
   }

   if (configData["data-field"]) {
      cfg_data_field = decodeURIComponent(configData["data-field"]);
   }

   if (configData.url) {
      cfg_endpoint = decodeURIComponent(configData.url);
   }

   const eventArray = configData["event-list"] !== "" ? configData["event-list"].split(",") : [];

   var dict = {
      1000: eventArray.length,
   };

   if (configData["begin-prefix"] !== null) {
      dict[901] = configData["begin-prefix"];
   }
   if (configData["end-prefix"] !== null) {
      dict[902] = configData["end-prefix"];
   }

   for (var i = 0; i < eventArray.length; i++) {
      dict[1001 + i] = eventArray[i];
   }

   Pebble.sendAppMessage(dict, function() {
      console.log("Send successful: " + JSON.stringify(dict));
   }, function() {
      console.log("Send failed!");
   });
});
