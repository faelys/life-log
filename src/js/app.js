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
var cfg_sign_algo = "";
var cfg_sign_field = "";
var cfg_sign_field_format = "";
var cfg_sign_key = "";
var cfg_sign_key_format = "";

var to_send = [];
var senders = [new XMLHttpRequest(), new XMLHttpRequest()];
var i_sender = 1;
var jsSHA = require("/src/js/sha.js");

function sendPayload(payload) {
   var data = new FormData();
   data.append(cfg_data_field, payload);

   if (cfg_sign_field) {
      var sha = new jsSHA(cfg_sign_algo, "TEXT");
      sha.setHMACKey(cfg_sign_key, cfg_sign_key_format);
      sha.update(payload);
      data.append(cfg_sign_field, sha.getHMAC(cfg_sign_field_format));
   }

   if (cfg_extra_fields.length > 0) {
      for (var i = 0; i < cfg_extra_fields.length; i += 1) {
         var decoded = decodeURIComponent(cfg_extra_fields[i]).split("=");
         var name = decoded.shift();
         var value = decoded.join("=");
         data.append(name, value);
      }
   }

   i_sender = 1 - i_sender;
   senders[i_sender].open("POST", cfg_endpoint, true);
   senders[i_sender].send(data);
}

function sendHead() {
   if (to_send.length < 1) return;
   sendPayload(to_send[0].split(";")[1]);
}

function enqueue(key, line) {
   to_send.push(key + ";" + line);
   localStorage.setItem("toSend", to_send.join("|"));
   if (to_send.length === 1) {
      sendHead();
   }
}

function uploadDone() {
   var sent_key = to_send.shift().split(";")[0];
   localStorage.setItem("toSend", to_send.join("|"));
   localStorage.setItem("lastSent", sent_key);
   sendHead();
}

function uploadError() { console.log(this.statusText); }

senders[0].addEventListener("load", uploadDone);
senders[0].addEventListener("error", uploadError);
senders[1].addEventListener("load", uploadDone);
senders[1].addEventListener("error", uploadError);

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

   if (cfg_sign_field) {
      result += "&s_algo=" + encodeURIComponent(cfg_sign_algo)
       + "&s_field=" + encodeURIComponent(cfg_sign_field)
       + "&s_fieldf=" + encodeURIComponent(cfg_sign_field_format)
       + "&s_key=" + encodeURIComponent(cfg_sign_key)
       + "&s_keyf=" + encodeURIComponent(cfg_sign_key_format);
   }

   return result;
}

Pebble.addEventListener("ready", function() {
   var str_to_send = localStorage.getItem("toSend");
   to_send = str_to_send ? str_to_send.split("|") : [];

   var str_extra_fields = localStorage.getItem("extra-fields");
   cfg_extra_fields = str_extra_fields ? str_extra_fields.split(",") : [];

   cfg_endpoint = localStorage.getItem("url");
   cfg_data_field = localStorage.getItem("data-field");
   cfg_sign_algo = localStorage.getItem("cfgSignAlgorithm");
   cfg_sign_field = localStorage.getItem("cfgSignFieldName");
   cfg_sign_field_format = localStorage.getItem("cfgSignFieldFormat");
   cfg_sign_key = localStorage.getItem("cfgSignKey");
   cfg_sign_key_format = localStorage.getItem("cfgSignKeyFormat");

   if (to_send.length >= 1) {
      sendHead();
   }

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
      localStorage.setItem("data-field", cfg_data_field);
   }

   if (configData.url) {
      cfg_endpoint = decodeURIComponent(configData.url);
      localStorage.setItem("url", cfg_endpoint);
   }

   if (configData.signAlgorithm) {
      cfg_sign_algo = configData.signAlgorithm;
      localStorage.setItem("cfgSignAlgorithm", cfg_sign_algo);
   }

   if (configData.signFieldName) {
      cfg_sign_field = configData.signFieldName;
      localStorage.setItem("cfgSignFieldName", cfg_sign_field);
   }

   if (configData.signFieldFormat) {
      cfg_sign_field_format = configData.signFieldFormat;
      localStorage.setItem("cfgSignFieldFormat", cfg_sign_field_format);
   }

   if (configData.signKey) {
      cfg_sign_key = decodeURIComponent(configData.signKey);
      localStorage.setItem("cfgSignKey", cfg_sign_key);
   }

   if (configData.signKeyFormat) {
      cfg_sign_key_format = configData.signKeyFormat;
      localStorage.setItem("cfgSignKeyFormat", cfg_sign_key_format);
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

Pebble.addEventListener("appmessage", function(e) {
   if (e.payload[500] && e.payload[510]) {
      enqueue(e.payload[500], e.payload[510]);
   }
});
