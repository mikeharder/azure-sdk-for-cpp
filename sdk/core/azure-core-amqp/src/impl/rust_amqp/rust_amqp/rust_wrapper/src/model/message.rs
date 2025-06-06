// Copyright (c) Microsoft Corporation. All Rights Reserved.
// Licensed under the MIT License.

// cspell: words amqp amqpvalue repr messagebuilder

use crate::{
    call_context::{call_context_from_ptr_mut, RustCallContext},
    error_from_str,
    model::{header::RustMessageHeader, properties::RustMessageProperties, value::RustAmqpValue},
};
use azure_core_amqp::{
    messaging::{
        builders::AmqpMessageBuilder, AmqpAnnotationKey, AmqpAnnotations,
        AmqpApplicationProperties, AmqpMessage, AmqpMessageBody,
    },
    value::{AmqpOrderedMap, AmqpValue},
    Deserializable,
};
use std::mem;
use tracing::warn;

#[repr(C)]
enum RustAmqpMessageBodyType {
    Data,
    Value,
    Sequence,
    None,
}

pub struct RustAmqpMessage {
    inner: AmqpMessage,
}

pub struct RustAmqpMessageBuilder {
    inner: AmqpMessageBuilder,
}

impl RustAmqpMessage {
    pub fn get(&self) -> &AmqpMessage {
        &self.inner
    }
}

#[no_mangle]
extern "C" fn messagebuilder_create() -> *mut RustAmqpMessageBuilder {
    Box::into_raw(Box::new(RustAmqpMessageBuilder {
        inner: AmqpMessage::builder(),
    }))
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_destroy(message_builder: *mut RustAmqpMessageBuilder) {
    mem::drop(Box::from_raw(message_builder));
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_build_and_destroy(
    message_builder: *mut RustAmqpMessageBuilder,
) -> *mut RustAmqpMessage {
    let message_builder = Box::from_raw(message_builder);
    let message = message_builder.inner.build();
    Box::into_raw(Box::new(RustAmqpMessage { inner: message }))
}

#[no_mangle]
extern "C" fn message_create() -> *mut RustAmqpMessage {
    Box::into_raw(Box::new(RustAmqpMessage {
        inner: Default::default(),
    }))
}

impl From<AmqpMessage> for RustAmqpMessage {
    fn from(inner: AmqpMessage) -> Self {
        RustAmqpMessage { inner }
    }
}

#[no_mangle]
unsafe extern "C" fn message_destroy(message: *mut RustAmqpMessage) {
    mem::drop(Box::from_raw(message));
}

#[no_mangle]
unsafe extern "C" fn message_deserialize(
    data: *const u8,
    count: usize,
    message: *mut *mut RustAmqpMessage,
) -> i32 {
    let data = std::slice::from_raw_parts(data, count);
    let val = AmqpMessage::decode(data);
    match val {
        Ok(m) => {
            *message = Box::into_raw(Box::new(RustAmqpMessage { inner: m }));
            0
        }
        Err(err) => {
            warn!("Failed to deserialize message: {:?}", err);
            println!("Failed to deserialize message: {:?}", err);
            1
        }
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_header(
    message: *const RustAmqpMessage,
    message_header: *mut *mut RustMessageHeader,
) -> i32 {
    let message = &*message;
    if message.inner.header().is_none() {
        *message_header = std::ptr::null_mut();
        return 0;
    }
    let header = message.inner.header().unwrap();
    *message_header = Box::into_raw(Box::new(RustMessageHeader::new(header.clone())));
    0
}

#[no_mangle]
unsafe extern "C" fn message_get_properties(
    message: *const RustAmqpMessage,
    message_properties: *mut *mut RustMessageProperties,
) -> i32 {
    let message = { &*message };
    let properties = message.inner.properties();
    let properties = properties.map(|p| Box::new(RustMessageProperties { inner: p.clone() }));
    match properties {
        Some(p) => *message_properties = Box::into_raw(p),
        None => *message_properties = std::ptr::null_mut(),
    }
    0
}

#[no_mangle]
unsafe extern "C" fn message_get_delivery_annotations(
    message: *const RustAmqpMessage,
    delivery_annotations: *mut *mut RustAmqpValue,
) -> i32 {
    let message = &*message;
    let amqp_da = message.inner.delivery_annotations();
    match amqp_da {
        Some(da) => {
            let map: AmqpOrderedMap<AmqpValue, AmqpValue> =
                da.0.clone()
                    .into_iter()
                    .map(|f| (AmqpValue::from(f.0), f.1))
                    .collect();
            *delivery_annotations = Box::into_raw(Box::new(RustAmqpValue {
                inner: AmqpValue::Map(map),
            }));
            0
        }
        None => {
            *delivery_annotations = std::ptr::null_mut();
            1
        }
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_message_annotations(
    message: *const RustAmqpMessage,
    delivery_annotations: *mut *mut RustAmqpValue,
) -> i32 {
    let message = &*message;
    let amqp_da = message.inner.message_annotations();
    match amqp_da {
        Some(da) => {
            let map: AmqpOrderedMap<AmqpValue, AmqpValue> =
                da.0.clone()
                    .into_iter()
                    .map(|f| (f.0.into(), f.1))
                    .collect();
            *delivery_annotations = Box::into_raw(Box::new(RustAmqpValue {
                inner: AmqpValue::Map(map),
            }));
            0
        }
        None => {
            *delivery_annotations = std::ptr::null_mut();
            1
        }
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_application_properties(
    message: *const RustAmqpMessage,
    application_properties: *mut *mut RustAmqpValue,
) -> i32 {
    let message = &*message;
    let amqp_da = message.inner.application_properties();
    match amqp_da {
        Some(da) => {
            let map: AmqpOrderedMap<AmqpValue, AmqpValue> =
                da.0.clone()
                    .into_iter()
                    .map(|f| (f.0.into(), f.1))
                    .collect();

            *application_properties = Box::into_raw(Box::new(RustAmqpValue {
                inner: AmqpValue::Map(map),
            }));
            0
        }
        None => {
            *application_properties = std::ptr::null_mut();
            1
        }
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_footer(
    message: *const RustAmqpMessage,
    footer: *mut *mut RustAmqpValue,
) -> i32 {
    let message = &*message;
    let amqp_da = message.inner.footer();
    match amqp_da {
        Some(da) => {
            let map: AmqpOrderedMap<AmqpValue, AmqpValue> =
                da.0.clone()
                    .into_iter()
                    .map(|f| (f.0.into(), f.1))
                    .collect();

            *footer = Box::into_raw(Box::new(RustAmqpValue {
                inner: AmqpValue::Map(map),
            }));
            0
        }
        None => {
            *footer = std::ptr::null_mut();
            1
        }
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_body_type(
    message: *const RustAmqpMessage,
    body_type: &mut RustAmqpMessageBodyType,
) -> i32 {
    let message = &*message;
    match message.inner.body() {
        AmqpMessageBody::Binary(_) => *body_type = RustAmqpMessageBodyType::Data,
        AmqpMessageBody::Value(_) => *body_type = RustAmqpMessageBodyType::Value,
        AmqpMessageBody::Sequence(_) => *body_type = RustAmqpMessageBodyType::Sequence,
        AmqpMessageBody::Empty => *body_type = RustAmqpMessageBodyType::None,
    }
    0
}

#[no_mangle]
unsafe extern "C" fn message_get_body_amqp_data_count(
    message: *const RustAmqpMessage,
    count: &mut usize,
) -> i32 {
    let message = &*message;
    match message.inner.body() {
        AmqpMessageBody::Binary(data) => *count = data.len(),
        _ => return 1,
    }
    0
}

#[no_mangle]
unsafe extern "C" fn message_get_body_amqp_sequence_count(
    message: *const RustAmqpMessage,
    count: &mut usize,
) -> i32 {
    let message = &*message;
    match message.inner.body() {
        AmqpMessageBody::Sequence(data) => *count = data.len(),
        _ => return 1,
    }
    0
}

#[no_mangle]
unsafe extern "C" fn message_get_body_amqp_data_in_place(
    message: *const RustAmqpMessage,
    index: u32,
    data: *mut *mut u8,
    count: *mut u32,
) -> i32 {
    let message = &*message;
    match message.inner.body() {
        AmqpMessageBody::Binary(d) => {
            *data = d[index as usize].as_ptr() as *mut u8;
            *count = d[index as usize].len() as u32;
            0
        }
        _ => 1,
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_body_amqp_sequence_in_place(
    message: *const RustAmqpMessage,
    index: u32,
    data: *mut *mut RustAmqpValue,
) -> i32 {
    let message = &*message;
    match message.inner.body() {
        AmqpMessageBody::Sequence(d) => {
            *data = Box::into_raw(Box::new(RustAmqpValue {
                inner: AmqpValue::List(d[index as usize].clone()),
            }));
            0
        }
        _ => 1,
    }
}

#[no_mangle]
unsafe extern "C" fn message_get_body_amqp_value_in_place(
    message: *const RustAmqpMessage,
    data: *mut *mut RustAmqpValue,
) -> i32 {
    let message = &*message;
    match message.inner.body() {
        AmqpMessageBody::Value(d) => {
            *data = Box::into_raw(Box::new(RustAmqpValue { inner: d.clone() }));
            0
        }
        _ => 1,
    }
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_delivery_annotations(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    delivery_annotations: *const RustAmqpValue,
) -> *mut RustAmqpMessageBuilder {
    let call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let delivery_annotations = &*delivery_annotations;
    if let AmqpValue::Map(map) = &delivery_annotations.inner {
        let amqp_map: AmqpOrderedMap<AmqpAnnotationKey, AmqpValue> =
            map.iter().map(|f| (f.0.into(), f.1)).collect();
        Box::into_raw(Box::new(RustAmqpMessageBuilder {
            inner: message_builder
                .inner
                .with_delivery_annotations(AmqpAnnotations(amqp_map)),
        }))
    } else {
        call_context.set_error(error_from_str("Delivery annotations must be a map"));
        std::ptr::null_mut()
    }
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_message_annotations(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    message_annotations: *const RustAmqpValue,
) -> *mut RustAmqpMessageBuilder {
    let call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let message_annotations = &*message_annotations;
    if let AmqpValue::Map(map) = &message_annotations.inner {
        let amqp_map: AmqpOrderedMap<AmqpAnnotationKey, AmqpValue> =
            map.iter().map(|f| (f.0.into(), f.1)).collect();
        Box::into_raw(Box::new(RustAmqpMessageBuilder {
            inner: message_builder
                .inner
                .with_message_annotations(AmqpAnnotations(amqp_map)),
        }))
    } else {
        call_context.set_error(error_from_str("Message annotations must be a map"));
        std::ptr::null_mut()
    }
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_application_properties(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    application_properties: *const RustAmqpValue,
) -> *mut RustAmqpMessageBuilder {
    let call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let application_properties = &*application_properties;
    if let AmqpValue::Map(map) = &application_properties.inner {
        let amqp_map: AmqpOrderedMap<String, AmqpValue> = map
            .iter()
            .map(|f| {
                (
                    match f.0 {
                        AmqpValue::String(s) => s,
                        _ => panic!("Application Properties key must be a string."),
                    },
                    f.1,
                )
            })
            .collect();
        Box::into_raw(Box::new(RustAmqpMessageBuilder {
            inner: message_builder
                .inner
                .with_application_properties(AmqpApplicationProperties(amqp_map)),
        }))
    } else {
        call_context.set_error(error_from_str("Application Properties must be a map"));
        std::ptr::null_mut()
    }
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_footer(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    footer: *const RustAmqpValue,
) -> *mut RustAmqpMessageBuilder {
    let call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let footer = &*footer;
    if let AmqpValue::Map(map) = &footer.inner {
        let amqp_map: AmqpOrderedMap<AmqpAnnotationKey, AmqpValue> =
            map.iter().map(|f| (f.0.into(), f.1)).collect();
        Box::into_raw(Box::new(RustAmqpMessageBuilder {
            inner: message_builder.inner.with_footer(AmqpAnnotations(amqp_map)),
        }))
    } else {
        call_context.set_error(error_from_str("Footer must be a map"));
        std::ptr::null_mut()
    }
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_header(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    message_header: *const RustMessageHeader,
) -> *mut RustAmqpMessageBuilder {
    let _call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let header = &*message_header;
    Box::into_raw(Box::new(RustAmqpMessageBuilder {
        inner: message_builder.inner.with_header(header.get().clone()),
    }))
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_properties(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    message_properties: *const RustMessageProperties,
) -> *mut RustAmqpMessageBuilder {
    let _call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let properties = &*message_properties;
    Box::into_raw(Box::new(RustAmqpMessageBuilder {
        inner: message_builder
            .inner
            .with_properties(properties.inner.clone()),
    }))
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_add_body_amqp_data(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    data: *const u8,
    count: usize,
) -> *mut RustAmqpMessageBuilder {
    let _call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let mut vec = Vec::new();
    vec.extend_from_slice(std::slice::from_raw_parts(data, count));
    Box::into_raw(Box::new(RustAmqpMessageBuilder {
        inner: message_builder.inner.add_message_body_binary(vec),
    }))
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_add_body_amqp_sequence(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    data: *const RustAmqpValue,
) -> *mut RustAmqpMessageBuilder {
    let call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let data = &*data;
    if let AmqpValue::List(list) = &data.inner {
        Box::into_raw(Box::new(RustAmqpMessageBuilder {
            inner: message_builder
                .inner
                .add_message_body_sequence(list.clone()),
        }))
    } else {
        call_context.set_error(error_from_str("Sequence must be a list"));
        std::ptr::null_mut()
    }
}

#[no_mangle]
unsafe extern "C" fn messagebuilder_set_body_amqp_value(
    call_context: *mut RustCallContext,
    message_builder: *mut RustAmqpMessageBuilder,
    data: *const RustAmqpValue,
) -> *mut RustAmqpMessageBuilder {
    let _call_context = call_context_from_ptr_mut(call_context);
    let message_builder = Box::from_raw(message_builder);
    let data = &*data;
    Box::into_raw(Box::new(RustAmqpMessageBuilder {
        inner: message_builder.inner.with_body(data.inner.clone()),
    }))
}
