use serde::Deserialize;
use serde_pickle::Value;
use anyhow::{Result, bail};
use std::collections::BTreeMap;

#[derive(Debug)]
pub enum RenpyStatement {
    Define(Define),
    Default(Default),
    Say(Say),
    Init(Init),
    Label(Label),
    Python(Python),
    Image(Image),
    Transform(Transform),
    Show(Show),
    Scene(Scene),
    Hide(Hide),
    Pass(Pass),
    Return(Return),
    If(If),
    While(While),
    Jump(Jump),
    Call(Call),
    Unknown(BTreeMap<serde_pickle::HashableValue, Value>),
}

#[derive(Debug)]
pub struct Define {
    pub varname: String,
    pub store: String,
}

#[derive(Debug)]
pub struct Default {
    pub varname: String,
    pub store: String,
}

#[derive(Debug, Deserialize)]
pub struct Say {
    pub what: String,
}

#[derive(Debug)]
pub struct Init {
    pub priority: i64,
    pub block: Vec<RenpyStatement>,
}

#[derive(Debug, Deserialize)]
pub struct Label {
    pub name: String,
}

#[derive(Debug, Deserialize)]
pub struct Python {
    pub source: String,
}

#[derive(Debug, Deserialize)]
pub struct Image {
    pub imgname: Vec<String>,
}

#[derive(Debug, Deserialize)]
pub struct Transform {
    pub varname: String,
}

#[derive(Debug, Deserialize)]
pub struct Show {
    pub imspec: Value,
}

#[derive(Debug, Deserialize)]
pub struct Scene {
    pub imspec: Option<Value>,
}

#[derive(Debug, Deserialize)]
pub struct Hide {
    pub imspec: Value,
}

#[derive(Debug, Deserialize)]
pub struct Pass;

#[derive(Debug, Deserialize)]
pub struct Return;

#[derive(Debug, Deserialize)]
pub struct If;

#[derive(Debug, Deserialize)]
pub struct While {
    pub condition: String,
}

#[derive(Debug, Deserialize)]
pub struct Jump {
    pub target: String,
}

#[derive(Debug, Deserialize)]
pub struct Call {
    pub label: String,
}

// Helper to extract the statements list from the top-level structure
pub fn extract_statements(val: &Value) -> Option<&Vec<Value>> {
    match val {
        Value::List(list) => {
             if list.len() >= 2 {
                 match &list[1] {
                     Value::List(stmts) => Some(stmts),
                     _ => None
                 }
             } else {
                 None
             }
        },
        Value::Tuple(list) => {
             if list.len() >= 2 {
                 match &list[1] {
                     Value::List(stmts) => Some(stmts),
                     _ => None
                 }
             } else {
                 None
             }
        },
        _ => None
    }
}

// Convert a single statement Value into RenpyStatement
pub fn parse_statement(val: &Value) -> Result<RenpyStatement> {
    let dict = match val {
        Value::List(list) => {
            if list.len() >= 2 {
                match &list[1] {
                    Value::Dict(d) => d,
                    _ => bail!("Second element of node is not a dict"),
                }
            } else {
                bail!("Node list too short");
            }
        },
        _ => bail!("Node is not a list"),
    };

    let get_str = |key: &str| -> Option<String> {
        dict.get(&serde_pickle::HashableValue::String(key.to_string()))
            .and_then(|v| match v {
                Value::String(s) => Some(s.clone()),
                _ => None
            })
    };

    // Init
    if let Some(priority) = dict.get(&serde_pickle::HashableValue::String("priority".to_string())) {
        if let Some(block_val) = dict.get(&serde_pickle::HashableValue::String("block".to_string())) {
             let mut block = Vec::new();
             // Check direct list
             if let Value::List(nodes) = block_val {
                  for node in nodes {
                       if let Ok(stmt) = parse_statement(node) {
                           block.push(stmt);
                       }
                  }
             }

             let prio = match priority {
                 Value::I64(i) => *i,
                 _ => 0,
             };
             return Ok(RenpyStatement::Init(Init { priority: prio, block }));
        }
    }

    if let Some(_) = get_str("operator") {
        let varname = get_str("varname").unwrap_or_default();
        let store = get_str("store").unwrap_or_default();
        return Ok(RenpyStatement::Define(Define { varname, store }));
    }
    
    if let Some(var) = get_str("varname") {
        if get_str("operator").is_none() {
             let store = get_str("store").unwrap_or_default();
             return Ok(RenpyStatement::Default(Default { varname: var, store }));
        }
    }
    
    if let Some(what) = get_str("what") {
         return Ok(RenpyStatement::Say(Say { what }));
    }

    if let Some(name) = get_str("name") {
        if let Some(_) = dict.get(&serde_pickle::HashableValue::String("block".to_string())) {
             return Ok(RenpyStatement::Label(Label { name }));
        }
    }
    
    if dict.contains_key(&serde_pickle::HashableValue::String("expression".to_string())) {
         return Ok(RenpyStatement::Return(Return));
    }

    if let Some(_code) = dict.get(&serde_pickle::HashableValue::String("code".to_string())) {
        if let Some(_) = get_str("imgname") {
             return Ok(RenpyStatement::Image(Image{ imgname: vec![] })); 
        }
    }

    Ok(RenpyStatement::Unknown(dict.clone()))
}
